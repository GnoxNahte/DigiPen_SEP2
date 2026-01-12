// Enemy1.cpp
#include "Enemy1.h"   // or "Enemy1.h" (match your filename)
#include "AEEngine.h"
#include <cmath>

// No include needed (avoids the include-path issue). This matches how you call it in GameScene.cpp.
namespace QuickGraphics
{
    void DrawRect(float x, float y, float w, float h, u32 color);
}

// --------- small helpers ---------
static inline float LengthSq(const AEVec2& v)
{
    return v.x * v.x + v.y * v.y;
}

static inline AEVec2 Sub(const AEVec2& a, const AEVec2& b)
{
    return AEVec2{ a.x - b.x, a.y - b.y };
}

static inline AEVec2 Mul(const AEVec2& v, float s)
{
    return AEVec2{ v.x * s, v.y * s };
}

static inline AEVec2 NormalizeSafe(const AEVec2& v)
{
    const float lsq = LengthSq(v);
    if (lsq <= 1e-8f)
        return AEVec2{ 0.f, 0.f };

    const float invLen = 1.0f / std::sqrt(lsq);
    return AEVec2{ v.x * invLen, v.y * invLen };
}

// --------- Enemy1 implementation ---------

Enemy1::Enemy1(float initialPosX, float initialPosY)
{
    position.x = initialPosX;
    position.y = initialPosY;

    velocity = AEVec2{ 0.f, 0.f };

    // Tuned to your current world scale:
    // - floor width is ~10 units
    // - camera scale is 64
    // so speeds/ranges should be in small numbers.
    sizeX = 0.8f;
    sizeY = 0.8f;

    moveSpeed = 2.5f;    // units per second
    detectRadius = 6.0f; // units
}

void Enemy1::Update(const AEVec2& playerPos)
{
    const float dt = (float)AEFrameRateControllerGetFrameTime();

    // Detect player
    const AEVec2 toPlayer = Sub(playerPos, position);
    const float distSq = LengthSq(toPlayer);
    const float detectSq = detectRadius * detectRadius;

    playerDetected = (distSq <= detectSq);

    if (playerDetected)
    {
        // Chase player (1-bit dungeon style)
        const AEVec2 dir = NormalizeSafe(toPlayer);
        velocity = Mul(dir, moveSpeed);
    }
    else
    {
        // Idle when player is too far (you can add wander later if desired)
        velocity = AEVec2{ 0.f, 0.f };
    }

    // Integrate movement
    AEVec2 delta;
    AEVec2Scale(&delta, &velocity, dt);
    AEVec2Add(&position, &position, &delta);
}

void Enemy1::Render() const
{
    // Visual debug: red when detected, grey otherwise
    const u32 color = playerDetected ? 0xFFFF4040 : 0xFFB0B0B0;

    // DrawRect(x, y, width, height, color) -- matches your GameScene.cpp usage
    QuickGraphics::DrawRect(position.x, position.y, sizeX, sizeY, color);
}
