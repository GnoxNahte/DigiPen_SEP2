/*!
@file   EnemyBossStats.cpp
@author lim kang ping
@brief  This file implements the EnemyBossStats structure.
It loads boss stats such as health, damage, movement,
attack values, and teleport settings from a JSON file.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/

#include <rapidjson/document.h>
#include <iostream>
#include <string>
#include "EnemyBossStats.h"
#include "../../Utils/FileHelper.h"

EnemyBossStats::EnemyBossStats(std::string file)
    : file(file)
{
    LoadFileData();
}

void EnemyBossStats::LoadFileData()
{
    rapidjson::Document doc;
    bool success = FileHelper::TryReadJsonFile(file, doc);

    if (!success)
    {
        std::cout << "[EnemyBossStats] Failed to read file: " << file << '\n';
        return;
    }

    maxHP = doc["maxHP"].GetInt();
    attackDamage = doc["attackDamage"].GetInt();

    moveSpeed = doc["moveSpeed"].GetFloat();
    aggroRange = doc["aggroRange"].GetFloat();
    aggroYRange = doc["aggroYRange"].GetFloat();
    attackYRange = doc["attackYRange"].GetFloat();

    phase2HpThreshold = doc["phase2HpThreshold"].GetFloat();

    sizeX = doc["size"]["x"].GetFloat();
    sizeY = doc["size"]["y"].GetFloat();

    attackStartRange = doc["attack"]["startRange"].GetFloat();
    attackHitRange = doc["attack"]["hitRange"].GetFloat();
    attackCooldown = doc["attack"]["cooldown"].GetFloat();
    attackHitTimeNormalized = doc["attack"]["hitTimeNormalized"].GetFloat();
    attackBreakRange = doc["attack"]["breakRange"].GetFloat();

    meleeHitboxWidth = doc["meleeHitbox"]["width"].GetFloat();
    meleeHitboxHeight = doc["meleeHitbox"]["height"].GetFloat();
    meleeHitboxXInset = doc["meleeHitbox"]["xInset"].GetFloat();
    meleeHitboxYOffset = doc["meleeHitbox"]["yOffset"].GetFloat();

    teleportInterval = doc["teleport"]["interval"].GetFloat();
    teleportBehindOffset = doc["teleport"]["behindOffset"].GetFloat();
    teleportHalfRange = doc["teleport"]["halfRange"].GetFloat();
    teleportWallPadding = doc["teleport"]["wallPadding"].GetFloat();
    teleportMinPlayerGap = doc["teleport"]["minPlayerGap"].GetFloat();

    minHurtDuration = doc["hurt"]["minHurtDuration"].GetFloat();
    staggerResetDelay = doc["hurt"]["staggerResetDelay"].GetFloat();
}