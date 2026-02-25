#include "Enemy.h"

#include <cmath>
#include "../../Utils/QuickGraphics.h"
#include "../Camera.h"
#include <imgui.h>
#include "../UI.h"

// ---- Static helpers ----
float Enemy::GetAnimDurationSec(const Sprite& sprite, int stateIndex)
{
    if (stateIndex < 0 || stateIndex >= sprite.metadata.rows)
        return 0.f;

    const auto& s = sprite.metadata.stateInfoRows[stateIndex];
    return (float)s.frameCount * (float)s.timePerFrame;
}


bool Enemy::HasGroundAhead(MapGrid& map, float dirX) const
{
    const AEVec2 hbPos = GetHurtboxPos();   // center
    const AEVec2 hbSize = GetHurtboxSize();  // full size

    const float eps = 0.05f;

    // Probe a point just in front of the feet
    const float probeX = hbPos.x + dirX * (hbSize.x * 0.5f + eps);
    const float probeY = hbPos.y - hbSize.y * 0.5f - eps;

    // MapGrid already treats "not NONE" as solid
    return map.CheckPointCollision(probeX, probeY);
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
        c.attackBreakRange = 2.2f;  // how far before attack cancels
        c.attackStartRange = 1.4f;
        c.maxHp = 5;
        c.hideAfterDeath = true;
        c.attackDamage = 2;

        break;

    case Preset::Skeleton:
        c.spritePath = "Assets/Craftpix/Skeleton.png";
        c.renderScale = 2.f;
        c.runVelThreshold = 0.1f;   // FIX: old EnemyB used 8.0f (too high)
        c.maxHp = 10;
 
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

    //enemy life system
    hp = cfg.maxHp;
    dead = false;

}

// ---- Update ----
void Enemy::Update(const AEVec2& playerPos, MapGrid& map)
{
    const float dt = (float)AEFrameRateControllerGetFrameTime();
    if (dead)
    {
        // Advance animation until the final frame starts, then stop updating so it doesn't loop.
        if (deathTimeLeft > 0.f)
        {
            float tpf = sprite.metadata.stateInfoRows[cfg.animDeath].timePerFrame;
            if (tpf <= 0.f) tpf = 0.1f;

            // Only update while we're not yet in the "last frame window"
            if (deathTimeLeft > tpf)
                sprite.Update();

            deathTimeLeft -= dt;
            if (deathTimeLeft < 0.f) deathTimeLeft = 0.f;
        }
        if (deathTimeLeft <= 0.f && cfg.hideAfterDeath)
            hidden = true;

        return;
    }

    // Hurt lock: keep the HURT row visible long enough to notice (play full row once)
    if (hurtTimeLeft > 0.f)
    {
        hurtTimeLeft -= dt;

        // While hurt, stop attacking / moving and just play the hurt animation
        attack.Reset();
        velocity = AEVec2{ 0.f, 0.f };
        chasing = false;

        // Force hurt state while timer is active (prevents any override)
        sprite.SetState(cfg.animHurt);

 
        sprite.Update();
        return;
    }

    const float desiredStopDist =
        (attack.startRange > 0.05f) ? (attack.startRange - 0.05f) : attack.startRange;

    const float dx = playerPos.x - position.x;
    const float absDx = std::fabs(dx);

    // --- Guard/leash ---
    const float playerFromHome = std::fabs(playerPos.x - homePos.x);
    const float enemyFromHome = std::fabs(position.x - homePos.x);

    const bool inAggroRange = (absDx <= cfg.aggroRange);

    // Hysteresis so we don't spam switch at the boundary
    const float leashEnter = cfg.leashRange + 0.01f;  // when to START returning
    const float leashExit = cfg.leashRange - 0.25f;  // when returning can be CANCELLED

    // ENTER return-home mode if player OR enemy goes beyond leash
    if (!returningHome)
    {
        if (playerFromHome > leashEnter || enemyFromHome > leashEnter)
            returningHome = true;
    }
    else
    {
        if (inAggroRange && playerFromHome <= cfg.leashRange && enemyFromHome <= leashExit)
            returningHome = false;
    }

    if (returningHome)
    {
        // Allow attacks while returning home (no chasing)
        if (inAggroRange)
        {
            if (dx != 0.f)
                facingDirection = AEVec2{ (dx > 0.f) ? 1.f : -1.f, 0.f };

            const float attackDur = GetAnimDurationSec(sprite, cfg.animAttack);
            attack.Update(dt, absDx, attackDur);

            if (attack.IsAttacking())
            {
                velocity = AEVec2{ 0.f, 0.f };
                UpdateAnimation();
                sprite.Update();
                return;
            }
        }
        else
        {
            attack.Reset();
        }

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

            if (!HasGroundAhead(map, dirX))
            {
                nextPos.x = position.x;
                velocity.x = 0.f;
            }
            position = nextPos;
        }

        UpdateAnimation();
        sprite.Update();
        return;
    }

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

            AEVec2 displacement;
            AEVec2Scale(&displacement, &velocity, dt);
            AEVec2 nextPos = position;
            AEVec2Add(&nextPos, &position, &displacement);

            if (chasing)
            {
                const float dirX = (dx > 0.f) ? 1.f : -1.f;
                if (!HasGroundAhead(map, dirX))
                {
                    nextPos.x = position.x;
                    velocity.x = 0.f;
                    chasing = false;
                }
                const float targetX = playerPos.x - dirX * desiredStopDist;

                if (dirX > 0.f && nextPos.x > targetX) { nextPos.x = targetX; velocity.x = 0.f; }
                if (dirX < 0.f && nextPos.x < targetX) { nextPos.x = targetX; velocity.x = 0.f; }
            }

            const float minX = homePos.x - cfg.leashRange;
            const float maxX = homePos.x + cfg.leashRange;

            if (nextPos.x < minX) { nextPos.x = minX; velocity.x = 0.f; }
            if (nextPos.x > maxX) { nextPos.x = maxX; velocity.x = 0.f; }

            position = nextPos;
        }
        else
        {
            chasing = false;
            velocity.x = 0.f;
            velocity.y = 0.f;
        }
    }

    UpdateAnimation();
    sprite.Update();
}

bool Enemy::TryTakeDamage(int dmg)
{
    if (dead || dmg <= 0 || hurtTimeLeft > 0.f) return false;

    // : prevent repeated hits from the SAME attack swing
    /*if (attackInstanceId >= 0 && attackInstanceId == lastHitAttackId)
        return false;

    if (attackInstanceId >= 0)
        lastHitAttackId = attackInstanceId;*/

    // --- The rest is in existing ApplyDamage logic ---
    hp -= dmg;

    UI::GetDamageTextSpawner().SpawnDamageText(dmg, DAMAGE_TYPE_NORMAL, position);

    if (hp <= 0)
    {
        hp = 0;
        dead = true;
        hidden = false;

        attack.Reset();
        chasing = false;
        returningHome = false;
        velocity = AEVec2{ 0.f, 0.f };

        sprite.SetState(cfg.animDeath, false, nullptr);
        deathTimeLeft = GetAnimDurationSec(sprite, cfg.animDeath);
        if (deathTimeLeft <= 0.f)
            deathTimeLeft = 0.5f;
    }
    else
    {
        hurtTimeLeft = GetAnimDurationSec(sprite, cfg.animHurt);
        if (hurtTimeLeft <= 0.3f)
            hurtTimeLeft = 0.3f;

        attack.Reset();
        sprite.SetState(cfg.animHurt);
    }

    return true;
}

/*void Enemy::ApplyDamage(int dmg)
{
	(void)TryTakeDamage(dmg, -1);
}*/


// ---- Animation selection ----
void Enemy::UpdateAnimation()
{
    if (dead)
    {
       
        return;
    }

    if (hurtTimeLeft > 0.f)
    {
        sprite.SetState(cfg.animHurt);
        return;
    }

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



void Enemy::DrawInspector()
{
    ImGui::Begin("Enemy");

    if (ImGui::CollapsingHeader("Runtime"))
    {
        ImGui::DragFloat2("Position", &position.x, 0.1f);
        ImGui::DragFloat2("Velocity", &velocity.x, 0.1f);
        ImGui::Checkbox("Chasing", &chasing);
        ImGui::Checkbox("ReturningHome", &returningHome);
        ImGui::Checkbox("Dead", &dead);

        ImGui::SeparatorText("HP");
        ImGui::SliderInt("HP", &hp, 0, cfg.maxHp);
        ImGui::Text("MaxHP: %d", cfg.maxHp);
    }

    if (ImGui::CollapsingHeader("Config"))
    {
        ImGui::DragFloat("MoveSpeed", &cfg.moveSpeed, 0.05f, 0.f, 20.f);
        ImGui::DragFloat("AggroRange", &cfg.aggroRange, 0.05f, 0.f, 50.f);
        ImGui::DragFloat("LeashRange", &cfg.leashRange, 0.05f, 0.f, 50.f);

        ImGui::SeparatorText("Combat");
        ImGui::SliderInt("AttackDamage", &cfg.attackDamage, 0, 10);
        ImGui::DragFloat("AttackCooldown", &cfg.attackCooldown, 0.01f, 0.f, 5.f);

        ImGui::SeparatorText("Debug");
        ImGui::Checkbox("DebugDraw", &debugDraw);
    }

    ImGui::End();
}


bool Enemy::CheckIfClicked(const AEVec2& mousePos)
{
    return fabsf(position.x - mousePos.x) < size.x &&
    fabsf(position.y - mousePos.y) < size.y;

}

// ---- Render ----
void Enemy::Render()
{
    if (hidden) return;

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
        position.y - (0 - sprite.metadata.pivot.y)
    );

    // Camera scale
    AEMtx33ScaleApply(&transform, &transform, Camera::scale, Camera::scale);
    AEGfxSetTransform(transform.m);

    sprite.Render();

    if (debugDraw)

    {
        //const float boxYOffset = -0.25f; // negative = draw LOWER (
        const u32 color = chasing ? 0xFFFF4040 : 0xFFB0B0B0;
        const AEVec2 hb = GetHurtboxPos();
        QuickGraphics::DrawRect(hb.x, hb.y, size.x, size.y, color, AE_GFX_MDM_LINES_STRIP);
    }
}
