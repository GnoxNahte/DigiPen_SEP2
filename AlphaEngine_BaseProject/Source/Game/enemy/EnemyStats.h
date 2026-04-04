/*!
@file   EnemyStats.h
@author lim kang ping
@brief  This file defines the stats used by regular enemies.
It stores enemy values such as health, damage, movement,
attack settings, animation states, and file data.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
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