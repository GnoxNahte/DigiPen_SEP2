#pragma once

class IDamageable;
class EnemyManager;
class EnemyBoss;
class Player;

class AttackSystem
{
public:
    // Call this ONCE per frame from GameScene after UpdateAll() / boss.Update()
    void ApplyEnemyAttacksToPlayer(Player& player, EnemyManager& enemies, EnemyBoss* boss);
};