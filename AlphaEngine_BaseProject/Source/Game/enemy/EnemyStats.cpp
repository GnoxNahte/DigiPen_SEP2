
#include <rapidjson/document.h>
#include <iostream>
#include <string>

#include "EnemyStats.h"
#include "../../Utils/FileHelper.h"

EnemyStats::EnemyStats(std::string file)
    : file(file)
{
    LoadFileData();
}

void EnemyStats::LoadFileData()
{
    rapidjson::Document doc;
    bool success = FileHelper::TryReadJsonFile(file, doc);

    if (!success)
    {
        // Keep defaults if file load fails
        std::cout << "[EnemyStats] Failed to read file: " << file << '\n';
        return;
    }

    spritePath = doc["spritePath"].GetString();

    maxHp = doc["maxHp"].GetInt();
    attackDamage = doc["attackDamage"].GetInt();
    hideAfterDeath = doc["hideAfterDeath"].GetBool();

    renderScale = doc["renderScale"].GetFloat();

    moveSpeed = doc["moveSpeed"].GetFloat();
    aggroRange = doc["aggroRange"].GetFloat();
    leashRange = doc["leashRange"].GetFloat();

    aggroYRange = doc["aggroYRange"].GetFloat();
    attackYRange = doc["attackYRange"].GetFloat();

    runVelThreshold = doc["runVelThreshold"].GetFloat();

    attackStartRange = doc["attackStartRange"].GetFloat();
    attackHitRange = doc["attackHitRange"].GetFloat();
    attackCooldown = doc["attackCooldown"].GetFloat();
    attackHitTimeNormalized = doc["attackHitTimeNormalized"].GetFloat();
    attackBreakRange = doc["attackBreakRange"].GetFloat();

    animAttack = doc["animAttack"].GetInt();
    animRun = doc["animRun"].GetInt();
    animIdle = doc["animIdle"].GetInt();
    animHurt = doc["animHurt"].GetInt();
    animDeath = doc["animDeath"].GetInt();
}