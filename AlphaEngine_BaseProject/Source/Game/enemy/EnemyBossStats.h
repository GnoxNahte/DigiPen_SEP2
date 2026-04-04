/*!
@file   EnemyBossStats.h
@author lim kang ping
@brief  This file defines the stats used by the boss enemy.
It stores boss values such as health, damage, movement, attack settings,
teleport settings, and loads them from a data file.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/

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