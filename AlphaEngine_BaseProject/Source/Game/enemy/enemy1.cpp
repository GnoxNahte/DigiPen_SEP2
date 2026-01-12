#include "Enemy1.h"
#include <cmath>

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
    if (lsq <= 1e-8f) return AEVec2{ 0.f, 0.f };

    const float invLen = 1.0f / std::sqrt(lsq);
    return AEVec2{ v.x * invLen, v.y * invLen };
}

Enemy1::Enemy1(float initialPosX, float initialPosY)
{
    position.x = initialPosX;
    position.y = initialPosY;
}

void Enemy1::Update(const AEVec2& playerPos)
{
    const float dt = (float)AEFrameRateControllerGetFrameTime();

    const AEVec2 toPlayer = Sub(playerPos, position);
    const float distSq = LengthSq(toPlayer);

    const float detectSq = detectRadius * detectRadius;
    playerDetected = (distSq <= detectSq);

    if (playerDetected)
    {
        // Chase
        const AEVec2 dir = NormalizeSafe(toPlayer);
        velocity = Mul(dir, moveSpeed);
    }
    else
    {
        // Idle (for now)
        velocity = AEVec2{ 0.f, 0.f };
    }

    // Integrate
    AEVec2 disp;
    AEVec2Scale(&disp, &velocity, dt);
    AEVec2Add(&position, &position, &disp);
}

void Enemy1::Render() const
{
    // Grey when idle, red when player detected (debug clarity)
    const u32 color = playerDetected ? 0xFFFF4040 : 0xFFB0B0B0; // ARGB

    // IMPORTANT: QuickGraphics::Init() must be called once in your scene init.
    QuickGraphics::DrawRect(position, size, color);
}
