#include "Enemy.h"

#include <cmath>
#include "../../Utils/QuickGraphics.h"
#include "../Camera.h"

// ---- Static helpers ----
float Enemy::GetAnimDurationSec(const Sprite& sprite, int stateIndex)
{
    if (stateIndex < 0 || stateIndex >= sprite.metadata.rows)
        return 0.f;

    const auto& s = sprite.metadata.stateInfoRows[stateIndex];
    return (float)s.frameCount * (float)s.timePerFrame;
}

Enemy::Config Enemy::MakePreset(Preset preset)
{
    Config c{};

    switch (preset)
    {
    case Preset::Druid:
        c.spritePath = "Assets/Craftpix/Druid.png";
        c.renderScale = 4.f;
        c.runVelThreshold = 0.1f;
        c.attackHitRange = 1.6f;    // try 1.4–2.0
        c.attackStartRange = 1.8f;  // must be >= hitRange usually
        c.attackBreakRange = 2.2f;  // how far before attack cancels
        c.attackStartRange = 1.4f;

        break;

    case Preset::Skeleton:
        c.spritePath = "Assets/Craftpix/Skeleton.png";
        c.renderScale = 2.f;
        c.runVelThreshold = 0.1f;   // FIX: old EnemyB used 8.0f (too high)
        break;
    }

    // Defaults already match your old ctor values:
    // attackStartRange=1.2, hitRange=1.0, cooldown=0.8, hitTime=0.5

    // NOTE:
    // These anim indices assume your meta order is:
    // 0 ATTACK, 1 DEATH, 2 RUN, 3 IDLE, 4 HURT
    // If your Druid meta is in a different order, just change these numbers.
    c.animAttack = 0;
    c.animDeath = 1;
    c.animRun = 2;
    c.animIdle = 3;
    c.animHurt = 4;

    return c;
}

// ---- Ctors ----
Enemy::Enemy(Preset preset, float initialPosX, float initialPosY)
    : Enemy(MakePreset(preset), initialPosX, initialPosY)
{
}

Enemy::Enemy(const Config& cfgIn, float initialPosX, float initialPosY)
    : cfg(cfgIn)
    , sprite(cfg.spritePath)
{
    position = AEVec2{ initialPosX, initialPosY };
    homePos = position;
    velocity = AEVec2{ 0.f, 0.f };

    size = AEVec2{ 0.8f, 0.8f };
    facingDirection = AEVec2{ 1.f, 0.f };
    chasing = false;

    // Attack component setup (same as your old EnemyA/B)
    attack.startRange = cfg.attackStartRange;
    attack.hitRange = cfg.attackHitRange;
    attack.cooldown = cfg.attackCooldown;
    attack.hitTimeNormalized = cfg.attackHitTimeNormalized;
    attack.breakRange = cfg.attackBreakRange;
}

// ---- Update ----
void Enemy::Update(const AEVec2& playerPos)
{
    const float dt = (float)AEFrameRateControllerGetFrameTime();

    const float desiredStopDist =
        (attack.startRange > 0.05f) ? (attack.startRange - 0.05f) : attack.startRange;

    const float dx = playerPos.x - position.x;
    const float absDx = std::fabs(dx);

    // --- Guard/leash ---
    const float playerFromHome = std::fabs(playerPos.x - homePos.x);
    const float enemyFromHome = std::fabs(position.x - homePos.x);

    if (playerFromHome > cfg.leashRange || enemyFromHome > cfg.leashRange + 0.01f)
        returningHome = true;

    if (returningHome)
    {
        attack.Reset();
        chasing = false;

        const float eps = 0.05f;
        const float dh = homePos.x - position.x;
        const float absDh = std::fabs(dh);

        velocity.y = 0.f;

        if (absDh <= eps)
        {
            position.x = homePos.x;
            velocity.x = 0.f;
            returningHome = false;
        }
        else
        {
            const float dirX = (dh > 0.f) ? 1.f : -1.f;
            facingDirection = AEVec2{ dirX, 0.f };
            velocity.x = dirX * cfg.moveSpeed;

            AEVec2 displacement;
            AEVec2Scale(&displacement, &velocity, dt);
            AEVec2 nextPos = position;
            AEVec2Add(&nextPos, &position, &displacement);

            if (dirX > 0.f && nextPos.x > homePos.x) { nextPos.x = homePos.x; velocity.x = 0.f; }
            if (dirX < 0.f && nextPos.x < homePos.x) { nextPos.x = homePos.x; velocity.x = 0.f; }

            position = nextPos;
        }

        UpdateAnimation();
        sprite.Update();
        return;
    }

    // --- Aggro ---
    const bool inAggroRange = (absDx <= cfg.aggroRange);

    // Update attack component (needs attack anim duration)
    const float attackDur = GetAnimDurationSec(sprite, cfg.animAttack);
    attack.Update(dt, absDx, attackDur);

    // If attacking, stop movement
    if (attack.IsAttacking())
    {
        velocity.x = 0.f;
        velocity.y = 0.f;
        chasing = false;
    }
    else
    {
        if (inAggroRange)
        {
            chasing = (absDx > desiredStopDist);

            // Face player
            if (dx != 0.f)
                facingDirection = AEVec2{ (dx > 0.f) ? 1.f : -1.f, 0.f };

            if (chasing)
            {
                const float dirX = (dx > 0.f) ? 1.f : -1.f;
                velocity.x = dirX * cfg.moveSpeed;
            }
            else
            {
                velocity.x = 0.f;
            }

            velocity.y = 0.f;

            // Integrate + clamp beside player
            AEVec2 displacement;
            AEVec2Scale(&displacement, &velocity, dt);
            AEVec2 nextPos = position;
            AEVec2Add(&nextPos, &position, &displacement);

            if (chasing)
            {
                const float dirX = (dx > 0.f) ? 1.f : -1.f;
                const float targetX = playerPos.x - dirX * desiredStopDist;

                if (dirX > 0.f && nextPos.x > targetX) { nextPos.x = targetX; velocity.x = 0.f; }
                if (dirX < 0.f && nextPos.x < targetX) { nextPos.x = targetX; velocity.x = 0.f; }
            }

            // Clamp within guard area
            const float minX = homePos.x - cfg.leashRange;
            const float maxX = homePos.x + cfg.leashRange;

            if (nextPos.x < minX) { nextPos.x = minX; velocity.x = 0.f; }
            if (nextPos.x > maxX) { nextPos.x = maxX; velocity.x = 0.f; }

            position = nextPos;
        }
    }

    UpdateAnimation();
    sprite.Update();
}

// ---- Animation selection ----
void Enemy::UpdateAnimation()
{
    if (attack.IsAttacking())
    {
        sprite.SetState(cfg.animAttack);
        return;
    }

    if (std::fabs(velocity.x) > cfg.runVelThreshold)
        sprite.SetState(cfg.animRun);
    else
        sprite.SetState(cfg.animIdle);
}

// ---- Render ----
void Enemy::Render()
{
    AEMtx33 transform;

    const bool faceRight =
        (velocity.x != 0.f) ? (velocity.x > 0.f) : (facingDirection.x > 0.f);

    // Scale (flip X if facing left)
    AEMtx33Scale(&transform, faceRight ? cfg.renderScale : -cfg.renderScale, cfg.renderScale);

    // Pivot correction (same as your Player / EnemyA / EnemyB)
    AEMtx33TransApply(
        &transform,
        &transform,
        position.x - (0.5f - sprite.metadata.pivot.x),
        position.y - (0.75f - sprite.metadata.pivot.y)
    );

    // Camera scale
    AEMtx33ScaleApply(&transform, &transform, Camera::scale, Camera::scale);
    AEGfxSetTransform(transform.m);

    sprite.Render();

    if (debugDraw)
    {
        const u32 color = chasing ? 0xFFFF4040 : 0xFFB0B0B0;
        QuickGraphics::DrawRect(position.x, position.y, size.x, size.y, color, AE_GFX_MDM_LINES_STRIP);
    }
}
