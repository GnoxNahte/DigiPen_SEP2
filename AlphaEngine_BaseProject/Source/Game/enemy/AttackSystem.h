#pragma once
#include "AEVec2.h"
#include <vector>
#include "../../Utils/Sprite.h"
#include <memory>


class IDamageable;
class EnemyManager;
class EnemyBoss;
class Player;


class MapGrid; // for collision checks in hitbox updates, consider refactoring to pass as parameter instead
class AttackSystem
{
public:

    // Call this ONCE per frame from GameScene after UpdateAll() / boss.Update()
    void ApplyEnemyAttacksToPlayer(Player& player, EnemyManager& enemies, EnemyBoss* boss, MapGrid& map);
    void Render();
    void UpdateEnemyAttack(Player& player, EnemyManager& enemies, EnemyBoss* boss, MapGrid& map);
    //void SetDebugDraw(bool enabled) { debug = enabled; }

private:
    struct EnemySpawnedHitbox
    {
        AEVec2 position{};   // center of the box
        AEVec2 size{};       // full width / height
        int damage = 1;
        float lifetime = 0.12f;
        bool alreadyHit = false;
        bool faceRight = true;
        // visual
        std::unique_ptr<Sprite> sprite;
    };

    std::vector<EnemySpawnedHitbox> enemyHitboxes;
    bool debug = false;

   
};