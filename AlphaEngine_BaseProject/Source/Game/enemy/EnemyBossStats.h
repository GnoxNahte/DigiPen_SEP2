#pragma once
#include <string>

struct EnemyBossStats
{
    int maxHP;
    int attackDamage;

    float moveSpeed;
    float aggroRange;
    float aggroYRange;
    float attackYRange;

    float phase2HpThreshold;

    float sizeX;
    float sizeY;

    float attackStartRange;
    float attackHitRange;
    float attackCooldown;
    float attackHitTimeNormalized;
    float attackBreakRange;

    float meleeHitboxWidth;
    float meleeHitboxHeight;
    float meleeHitboxXInset;
    float meleeHitboxYOffset;

    float teleportInterval;
    float teleportBehindOffset;
    float teleportHalfRange;
    float teleportWallPadding;
    float teleportMinPlayerGap;

    float minHurtDuration;
    float staggerResetDelay;

    std::string file;

    EnemyBossStats(std::string file);
    void LoadFileData();
};