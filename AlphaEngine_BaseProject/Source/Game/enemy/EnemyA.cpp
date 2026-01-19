#include "EnemyA.h"
#include <cmath>
#include "AEEngine.h"
#include "../../Utils/QuickGraphics.h"
#include "../Camera.h"


EnemyA::EnemyA(float initialPosX, float initialPosY)
    : sprite("Assets/Craftpix/Alien6.png")  
{
    position = AEVec2{ initialPosX, initialPosY };
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

    const float dx = playerPos.x - position.x;
    const float absDx = std::fabs(dx);


    //chase detection
    const bool inAggroRange = (absDx <= aggroRange);


    // Update attack component
    const float attackDur = GetAnimDurationSec(sprite, ATTACK);
    attack.Update(dt, absDx, attackDur);

    // If attacking, stop movement. Else, do your existing chase logic.
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
            // Only chase if player is within aggro range
            chasing = (absDx > stopDistance);

            if (chasing)
            {
                const float dirX = (dx > 0.f) ? 1.f : -1.f;
                velocity.x = dirX * moveSpeed;
                facingDirection = AEVec2{ dirX, 0.f };
            }
            else
            {
                velocity.x = 0.f;
            }

            velocity.y = 0.f;

            AEVec2 displacement;
            AEVec2Scale(&displacement, &velocity, dt);
            AEVec2Add(&position, &position, &displacement);

            if (position.y < 0.f)
                position.y = 0.f;
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
        sprite.SetState(IDLE1);
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
        position.y - (0.5f - sprite.metadata.pivot.y)
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
