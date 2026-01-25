#include "EnemyBoss.h"
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


EnemyBoss::EnemyBoss(float initialPosX, float initialPosY)
    : sprite("Assets/Craftpix/Bringer_of_Death.png")
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

EnemyBoss::~EnemyBoss()
{
}

static float GetAnimDurationSec(const Sprite& sprite, int stateIndex)
{
    if (stateIndex < 0 || stateIndex >= sprite.metadata.rows)
        return 0.f;

    const auto& s = sprite.metadata.stateInfoRows[stateIndex];
    return (float)s.frameCount * (float)s.timePerFrame;
}


void EnemyBoss::Update(const AEVec2& playerPos)
{
    const float dt = (float)AEFrameRateControllerGetFrameTime();

    const float dx = playerPos.x - position.x;
    const float absDx = std::fabs(dx);


    //chase detection
    const bool inAggroRange = (absDx <= aggroRange);


    // Update attack component
    const float attackDur = GetAnimDurationSec(sprite, ATTACK);
    attack.Update(dt, absDx, attackDur);

    // Spawn one orb when attack starts (testing)!!!!!!!!!
    if (attack.JustStarted())
    {
        const float dir = (facingDirection.x >= 0.f) ? 1.f : -1.f;

        GlowOrb o;
        o.pos = AEVec2{ position.x + dir * 0.6f, position.y + 0.35f };
        o.vel = AEVec2{ dir * 7.0f, 0.f };      // speed in tiles/sec
        o.radius = 0.14f;
        o.life = 1.6f;
        o.color = 0xFFAA66FF;                   // purple glow (ARGB)

        g_orbs.push_back(o);
    }


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

    // Update + cleanup orbs(TESTING)!!!!!!
    for (auto& o : g_orbs) o.Update(dt);
    g_orbs.erase(std::remove_if(g_orbs.begin(), g_orbs.end(),
        [](const GlowOrb& o) { return !o.alive(); }), g_orbs.end());
}


void EnemyBoss::UpdateAnimation()
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




void EnemyBoss::Render()
{
    AEMtx33 m;

    const bool faceRight =
        (velocity.x != 0.f) ? (velocity.x > 0.f) : (facingDirection.x > 0.f);

    const float bossScale = 3.5f;

    AEMtx33Scale(&m, faceRight ? bossScale : -bossScale, bossScale);

    float px = sprite.metadata.pivot.x;
    float py = sprite.metadata.pivot.y;

    if (!faceRight)
        px = 1.0f - px;

    AEMtx33TransApply(
        &m, &m,
        position.x - (0.5f - px),
        position.y - (0.5f - py)
    );


    AEMtx33ScaleApply(&m, &m, Camera::scale, Camera::scale);
    AEGfxSetTransform(m.m);

    sprite.Render();

    // ---- RESET TRANSFORM FOR WORLD-SPACE PRIMITIVES ----
    AEMtx33 world;
    AEMtx33Scale(&world, Camera::scale, Camera::scale);
    AEGfxSetTransform(world.m);

    for (const auto& o : g_orbs)
        o.Render();

    if (debugDraw)
    {
        const u32 color = chasing ? 0xFFFF4040 : 0xFFB0B0B0;
        QuickGraphics::DrawRect(position.x, position.y, size.x, size.y, color, AE_GFX_MDM_LINES_STRIP);
    }
}

