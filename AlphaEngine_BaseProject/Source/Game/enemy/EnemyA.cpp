#include "EnemyA.h"
#include <cmath>
#include "../../Utils/QuickGraphics.h"
#include "../Camera.h"

#include <vector>

static inline u32 ScaleAlpha(u32 argb, float alphaMul)
{
    // argb = 0xAARRGGBB
    unsigned a = (argb >> 24) & 0xFF;
    unsigned rgb = argb & 0x00FFFFFF;

    float af = (a / 255.0f) * alphaMul;

    af = max(0.0f, min(1.0f, af));

    unsigned anew = (unsigned)(af * 255.0f + 0.5f);
    return (anew << 24) | rgb;
}



EnemyA::EnemyA(float initialPosX, float initialPosY)
    : sprite("Assets/Craftpix/Druid.png")  
{
    position = AEVec2{ initialPosX, initialPosY };
    homePos = position; // anchor guard point at spawn
    velocity = AEVec2{ 0.f, 0.f };



    // Make sure the enemy is visible by default
    size = AEVec2{ 0.8f, 0.8f };
    facingDirection = AEVec2{ 1.f, 0.f };
    chasing = false;

    //------may remove later, testing this attack system-----
    attack.startRange = 1.2f;
    attack.hitRange = 1.0f;
    attack.cooldown = 0.8f;
    attack.hitTimeNormalized = 0.5f;

    attack.breakRange = attack.startRange;

}

EnemyA::~EnemyA()
{
}

static float GetAnimDurationSec(const Sprite& sprite, int stateIndex)
{
    if (stateIndex < 0 || stateIndex >= sprite.metadata.rows)
        return 0.f;

    const auto& s = sprite.metadata.stateInfoRows[stateIndex];
    return (float)s.frameCount * (float)s.timePerFrame;
}


void EnemyA::Update(const AEVec2& playerPos)
{
    const float dt = (float)AEFrameRateControllerGetFrameTime();

    const float desiredStopDist = (attack.startRange > 0.05f) ? (attack.startRange - 0.05f)
        : attack.startRange;

    const float dx = playerPos.x - position.x;
    const float absDx = std::fabs(dx);

    // --- Guard/leash: do not chase if player baits too far from home ---
    const float playerFromHome = std::fabs(playerPos.x - homePos.x);
    const float enemyFromHome = std::fabs(position.x - homePos.x);

    // If player is outside guard area OR enemy somehow got dragged out, return home
    if (playerFromHome > leashRange || enemyFromHome > leashRange + 0.01f)
        returningHome = true;

    if (returningHome)
    {
        // Cancel combat while returning
        attack.Reset();
        chasing = false;

        const float eps = 0.05f;
        const float dh = homePos.x - position.x;
        const float absDh = std::fabs(dh);

        velocity.y = 0.f;

        if (absDh <= eps)
        {
            // Arrived home
            position.x = homePos.x;
            velocity.x = 0.f;
            returningHome = false;
        }
        else
        {
            // Move back home
            const float dirX = (dh > 0.f) ? 1.f : -1.f;
            facingDirection = AEVec2{ dirX, 0.f };
            velocity.x = dirX * moveSpeed;

            AEVec2 displacement;
            AEVec2Scale(&displacement, &velocity, dt);
            AEVec2 nextPos = position;
            AEVec2Add(&nextPos, &position, &displacement);

            // Clamp so we don't overshoot home
            if (dirX > 0.f && nextPos.x > homePos.x) { nextPos.x = homePos.x; velocity.x = 0.f; }
            if (dirX < 0.f && nextPos.x < homePos.x) { nextPos.x = homePos.x; velocity.x = 0.f; }

            position = nextPos;
        }

        UpdateAnimation();
        sprite.Update();
        return;
    }


    //chase detection
    const bool inAggroRange = (absDx <= aggroRange);




    // Update attack component
    const float attackDur = GetAnimDurationSec(sprite, ATTACK);
    attack.Update(dt, absDx, attackDur);




    // If attacking, stop movement. Else, do existing chase logic.
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

            // always face player
            if (dx != 0.f)
                facingDirection = AEVec2{ (dx > 0.f) ? 1.f : -1.f, 0.f };

            if (chasing)
            {
                const float dirX = (dx > 0.f) ? 1.f : -1.f;
                velocity.x = dirX * moveSpeed;
            }
            else
            {
                velocity.x = 0.f;
            }

            velocity.y = 0.f;

            // integrate + clamp so we stop beside player, not inside player
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

            // --- Clamp within guard area ---
            const float minX = homePos.x - leashRange;
            const float maxX = homePos.x + leashRange;

            if (nextPos.x < minX) { nextPos.x = minX; velocity.x = 0.f; }
            if (nextPos.x > maxX) { nextPos.x = maxX; velocity.x = 0.f; }

            position = nextPos;
        }

   

    }

    // Animation selection (attack overrides idle)
    UpdateAnimation();

    // Advance sprite frames
    sprite.Update();
}


void EnemyA::UpdateAnimation()
{
    if (attack.IsAttacking())
    {
        sprite.SetState(ATTACK); 
        return;
    }

    if (std::fabs(velocity.x) > 8.0f)
        sprite.SetState(RUN);
    else
        sprite.SetState(IDLE);
}




void EnemyA::Render() 
{
    AEMtx33 transform;
    // === Draw sprite using the same transform pipeline as Player ===
    const bool faceRight =
        (velocity.x != 0.f) ? (velocity.x > 0.f) : (facingDirection.x > 0.f);

    // Local scale (flip X if facing left). Match Player's 2.0f scaling.
    AEMtx33Scale(&transform, faceRight ? 2.f : -2.f, 2.f);

    // Pivot correction, same formula as Player
    AEMtx33TransApply(
        &transform,
        &transform,
        position.x - (0.5f - sprite.metadata.pivot.x),
        position.y - (0.75f - sprite.metadata.pivot.y)
    );

    // Camera scale (scales translation too), same as Player
    AEMtx33ScaleApply(&transform, &transform, Camera::scale, Camera::scale);
    AEGfxSetTransform(transform.m);

    // Draw the sprite
    sprite.Render();



    // === Debug draw rect (same approach as Player debug) ===
    if (debugDraw)
    {
        const u32 color = chasing ? 0xFFFF4040 : 0xFFB0B0B0;
        QuickGraphics::DrawRect(position.x, position.y, size.x, size.y, color, AE_GFX_MDM_LINES_STRIP);
    }
}
