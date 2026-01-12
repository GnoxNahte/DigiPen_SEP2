#pragma once
#include "AEEngine.h"

class EnemyA
{
public:
    EnemyA(float initialPosX = 0.f, float initialPosY = 0.f);
    ~EnemyA();

    void Update(const AEVec2& playerPos);
    void Render() const;

    AEVec2 position{ 0.f, 0.f }; // public, consistent with how GameScene uses player.position

private:
    AEVec2 velocity{ 0.f, 0.f };

    float width{ 0.8f };
    float height{ 0.8f };

    float moveSpeed{ 2.2f };
    float stopDistance{ 0.2f };

    bool chasing{ false };
};
#pragma once
