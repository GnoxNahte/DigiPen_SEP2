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
}

EnemyA::~EnemyA()
{
}

void EnemyA::Update(const AEVec2& playerPos)
{
    const float dt = (float)AEFrameRateControllerGetFrameTime();

    // Chase in X only, stay grounded
    const float dx = playerPos.x - position.x;
    const float absDx = std::fabs(dx);

    chasing = (absDx > stopDistance);

    if (chasing)
    {
        const float dirX = (dx > 0.f) ? 1.f : -1.f;
        velocity.x = dirX * moveSpeed;

        // Keep facing direction consistent with movement, like Player does
        facingDirection.x = dirX;
        facingDirection.y = 0.f;
    }
    else
    {
        velocity.x = 0.f;
    }

    // No vertical motion for now
    velocity.y = 0.f;

    // Integrate position
    AEVec2 displacement;
    AEVec2Scale(&displacement, &velocity, dt);
    AEVec2Add(&position, &position, &displacement);


    if (position.y < 0.f)
        position.y = 0.f;


    UpdateAnimation();   
    sprite.Update();
}

void EnemyA::UpdateAnimation()
{
    // Death should override 
    if (isDead)
    {
        sprite.SetState(DEATH, true);
        return;
    }

    // Jump 
    if (!isGrounded)
    {
        sprite.SetState(JUMP);
        return;
    }

    // Attack usually should lock until it finishes
    if (isAttacking)
    {
        sprite.SetState(ATTACK, true);
        return;
    }

    // Idle variants (swap if not moving)
    if (std::fabs(velocity.x) < 0.01f)
    {
       

        sprite.SetState(IDLE1);
    }
    else
    {

        sprite.SetState(RUN);
        idleSwapTimer = 0.f;
    }
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
