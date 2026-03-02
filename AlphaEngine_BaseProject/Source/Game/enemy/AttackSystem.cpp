#include "AttackSystem.h"

#include "../enemy/EnemyManager.h"
#include "../enemy/Enemy.h"
#include "../enemy/EnemyBoss.h"
#include "../Player/Player.h"



void AttackSystem::ApplyEnemyAttacksToPlayer(Player& player, EnemyManager& enemies, EnemyBoss* boss)
{
    const AEVec2 pPos = player.GetPosition();
    const AEVec2 pSize = player.GetStats().playerSize;

    // ---- Regular enemies ----
    enemies.ForEachEnemy([&](Enemy& e)
        {
            if (!e.PollAttackHit()) return;

            const AEVec2 ePos = e.GetPosition();

            const float dx = std::fabs(pPos.x - ePos.x);
            const float dy = std::fabs(pPos.y - ePos.y);

            // mid/close range on X, plus a small Y tolerance so it doesn't hit through floors
            if (dx <= e.GetAttackHitRange() && dy <= (pSize.y * 0.5f + 0.6f))
            {
                player.TakeDamage(e.GetAttackDamage(), e.GetPosition());
                std::cout << "Enemy HIT player!\n";
            }
        });

    // ---- Boss melee ----
    if (boss && !boss->IsDead())
    {
        if (boss->PollAttackHit())
        {
            player.TakeDamage(boss->GetAttackDamage(), boss->GetHurtboxPos());
            std::cout << "[Boss] HIT player (melee)\n";
        }

        // ---- Boss spell/projectile hits ----
        // ConsumeSpecialHits() does AABB overlap + consumes projectile so it won't hit twice.
        const int spellHits = boss->ConsumeSpecialHits(pPos, pSize);
        if (spellHits > 0)
        {

            const int spellDmg = 1;
            // Choose ONE of these policies:

            // Policy A (common): treat projectile hit this frame?as 1 damage event
            player.TakeDamage(spellHits * spellDmg, boss->GetHurtboxPos());
            std::cout << "[Boss] spell hit x" << spellHits << "\n";

            // Policy B (harsher): each projectile counts
            // player.TryTakeDamage(spellHits);  // only if your Player i-frames allow this to matter
        }
    }
}