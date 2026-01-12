#pragma once

#include "AEEngine.h"
namespace QuickGraphics
{
    void DrawRect(float x, float y, float w, float h, u32 color);
}

class Enemy1
{
public:
    Enemy1() = default;
    Enemy1(float initialPosX, float initialPosY);

    void Update(const AEVec2& playerPos);
    void Render() const;

    const AEVec2& GetPosition() const { return position; }

private:
    AEVec2 position{ 0.f, 0.f };
    AEVec2 velocity{ 0.f, 0.f };

    // Rectangle representation
    AEVec2 size{ 36.f, 36.f };

    // AI tuning (1-bit dungeon feel)
    float moveSpeed{ 90.f };
    float detectRadius{ 260.f };

    bool playerDetected{ false };
};
