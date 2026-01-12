#include "EnemyA.h"
#include <cmath>
#include "AEEngine.h"

// Avoid include-path issues: forward declare the function you already call in GameScene.cpp
class QuickGraphics
{
public:
    static void DrawRect(float posX, float posY, float scaleX, float scaleY, u32 color);
};

EnemyA::EnemyA(float initialPosX, float initialPosY)
{
    position.x = initialPosX;
    position.y = initialPosY;
    velocity = AEVec2{ 0.f, 0.f };
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
    }
    else
    {
        velocity.x = 0.f;
    }

    velocity.y = 0.f;

    // Integrate like Player.cpp
    AEVec2 displacement;
    AEVec2Scale(&displacement, &velocity, dt);
    AEVec2Add(&position, &position, &displacement);

    // Keep on ground (same convention as Player.cpp)
    if (position.y < 0.f)
        position.y = 0.f;
}

void EnemyA::Render() const
{
    const u32 color = chasing ? 0xFFFF4040 : 0xFFB0B0B0; // ARGB
    QuickGraphics::DrawRect(position.x, position.y, width, height, color);
}
