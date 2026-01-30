#include "EnemyA.h"
#include <cmath>
#include "AEEngine.h"
#include "../../Utils/QuickGraphics.h"
#include "../Camera.h"

#include <vector>
#include <algorithm>

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

static void DrawGlowBall_Local(float x, float y, float radius, u32 baseColorARGB)
{
    // If AE_GFX_BM_ADD doesn't exist in your AlphaEngine build, comment out the next line.
    AEGfxSetBlendMode(AE_GFX_BM_ADD);

    // Core
    QuickGraphics::DrawRect(x, y, radius * 2.0f, radius * 2.0f,
        ScaleAlpha(baseColorARGB, 1.0f), AE_GFX_MDM_TRIANGLES);

    // Glow layers
    QuickGraphics::DrawRect(x, y, radius * 3.0f, radius * 3.0f,
        ScaleAlpha(baseColorARGB, 0.22f), AE_GFX_MDM_TRIANGLES);

    QuickGraphics::DrawRect(x, y, radius * 4.5f, radius * 4.5f,
        ScaleAlpha(baseColorARGB, 0.12f), AE_GFX_MDM_TRIANGLES);

    QuickGraphics::DrawRect(x, y, radius * 6.5f, radius * 6.5f,
        ScaleAlpha(baseColorARGB, 0.06f), AE_GFX_MDM_TRIANGLES);

    // Restore normal blending
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
}

struct GlowOrb
{
    AEVec2 pos{ 0.f, 0.f };
    AEVec2 vel{ 0.f, 0.f };
    float  radius{ 0.16f };
    float  life{ 1.5f };
    u32    color{ 0xFF66CCFF }; // cyan

    bool alive() const { return life > 0.f; }

    void Update(float dt)
    {
        AEVec2 disp;
        AEVec2Scale(&disp, &vel, dt);
        AEVec2Add(&pos, &pos, &disp);
        life -= dt;
    }

    void Render() const
    {
        DrawGlowBall_Local(pos.x, pos.y, radius, color);
    }
};

// Global list for testing (fine while you only have 1 EnemyA / no boss yet)
static std::vector<GlowOrb> g_orbs;


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
