#pragma once
#include <string>

struct EnemyStats
{
    // ===== Stats loaded from JSON =====
    //mmanual numbers incase json/read fails?
    std::string spritePath;

    int maxHp;
    int attackDamage;
    bool hideAfterDeath;

    float renderScale;

    float moveSpeed;
    float aggroRange;
    float leashRange;

    float aggroYRange;
    float attackYRange;

    float runVelThreshold;

    float attackStartRange;
    float attackHitRange;
    float attackCooldown;
    float attackHitTimeNormalized;
    float attackBreakRange;

    int animAttack;
    int animRun;
    int animIdle;
    int animHurt;
    int animDeath;

    std::string file;

    EnemyStats(std::string file);

    void LoadFileData();
};