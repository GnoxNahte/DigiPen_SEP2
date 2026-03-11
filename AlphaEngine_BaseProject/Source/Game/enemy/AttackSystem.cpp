#include "AttackSystem.h"
#include "../enemy/EnemyManager.h"
#include "../enemy/Enemy.h"
#include "../enemy/EnemyBoss.h"
#include "../Player/Player.h"
#include "../../Utils/PhysicsUtils.h"
#include <utility>
#include "../../Utils/QuickGraphics.h"


//HELPERS
static float GetAnimDurationSec(const Sprite& sprite, int stateIndex)
{
    if (stateIndex < 0 || stateIndex >= sprite.metadata.rows)
        return 0.f;

    const auto& s = sprite.metadata.stateInfoRows[stateIndex];
    return static_cast<float>(s.frameCount) * static_cast<float>(s.timePerFrame);
}

static bool FindGroundBelowPlayer(MapGrid& map, float x, float startY, float minY, float step, float& outGroundY)
{
    for (float y = startY; y >= minY; y -= step)
    {
        if (map.CheckPointCollision(x, y))
        {
            outGroundY = y;
            return true;
        }
    }
    return false;
}


//PUBLIC APIIIII , to be called from gamescene and level editor

void AttackSystem::UpdateEnemyAttack(Player& player, EnemyManager& enemies, EnemyBoss* boss, MapGrid& map)
{
	ApplyEnemyAttacksToPlayer(player, enemies, boss, map);
    Render();

}

// This is the main function that applies all enemy attacks to the player each frame.
void AttackSystem::ApplyEnemyAttacksToPlayer(Player& player, EnemyManager& enemies, EnemyBoss* boss, MapGrid& map)
{
    const float dt = (float)AEFrameRateControllerGetFrameTime();

    const AEVec2 pPos = player.GetPosition();
    const AEVec2 pSize = player.GetStats().playerSize;


    // 1) Update existing spawned hitboxes first
    for (auto& hb : enemyHitboxes)
    {
        hb.lifetime -= dt;
        if (hb.sprite)
            hb.sprite->Update();


        if (!hb.alreadyHit && PhysicsUtils::AABB(hb.position, hb.size, pPos, pSize))
        {
            player.TryTakeDamage(hb.damage, hb.position);
            hb.alreadyHit = true;
        }
    }

    enemyHitboxes.erase(
        std::remove_if(enemyHitboxes.begin(), enemyHitboxes.end(),
            [](const EnemySpawnedHitbox& hb)
            {
                return hb.lifetime <= 0.f;
            }),
        enemyHitboxes.end()
    );

    // 2) Process enemy hit events
    enemies.ForEachEnemy([&](Enemy& e)
        {
            if (!e.PollAttackHit()) return;

            if (e.IsDruid())
            {
                EnemySpawnedHitbox hb;
                hb.size = { 1.2f, 0.45f };
                hb.position.x = pPos.x;
                hb.faceRight = (pPos.x >= e.GetPosition().x);

                float groundY = 0.0f;

                // start search from player's feet
                const float feetY = pPos.y - (pSize.y * 0.5f);

                // search downward for solid ground
                if (FindGroundBelowPlayer(map, pPos.x, feetY, feetY - 5.0f, 0.1f, groundY))
                {
                    // place spell on top of ground
                    hb.position.y = groundY + (hb.size.y * 0.5f);
                }
                else
                {
                    // fallback: don't spawn if no ground found
                    return;
                }

                hb.damage = e.GetAttackDamage();
                hb.alreadyHit = false;

                hb.sprite = std::make_unique<Sprite>("Assets/Craftpix/DruidEarth.png");
                constexpr int SPELL_STATE = 0;

                if (hb.sprite && SPELL_STATE >= 0 && SPELL_STATE < hb.sprite->metadata.rows)
                {
                    hb.sprite->SetState(SPELL_STATE, false, nullptr);
                    hb.lifetime = GetAnimDurationSec(*hb.sprite, SPELL_STATE);
                    if (hb.lifetime <= 0.f)
                        hb.lifetime = 0.5f;
                }
                else
                {
                    hb.lifetime = 0.5f;
                }

                if (PhysicsUtils::AABB(hb.position, hb.size, pPos, pSize))
                {
                    player.TryTakeDamage(hb.damage, hb.position);
                    hb.alreadyHit = true;
                }

                enemyHitboxes.push_back(std::move(hb));
                return;
            }

            // skeleton / normal enemy old behaviour
            const AEVec2 ePos = e.GetPosition();
            const float dx = std::fabs(pPos.x - ePos.x);
            const float dy = std::fabs(pPos.y - ePos.y);

            if (dx <= e.GetAttackHitRange() && dy <= (pSize.y * 0.5f + 0.6f))
            {
                player.TryTakeDamage(e.GetAttackDamage(), e.GetPosition());
            }
        });

    // ---- Boss melee ----
    if (boss && !boss->IsDead())
    {
        if (boss->PollAttackHit())
        {
            const auto& hb = boss->GetMeleeHitbox();
            if (PhysicsUtils::AABB(hb.position, hb.size, pPos, pSize))
            {
                player.TryTakeDamage(boss->GetAttackDamage(), boss->GetHurtboxPos());
                std::cout << "[Boss] HIT player (melee)\n";
            }
        
        }

        // ---- Boss spell/projectile hits ----
        // ConsumeSpecialHits() does AABB overlap + consumes projectile so it won't hit twice.
        const int spellHits = boss->ConsumeSpecialHits(pPos, pSize);
        if (spellHits > 0)
        {

            const int spellDmg = 1;
            player.TryTakeDamage(spellHits * spellDmg, boss->GetHurtboxPos());
            std::cout << "[Boss] spell hit x" << spellHits << "\n";
        }
    }
}

void AttackSystem::Render()
{
    AEMtx33 world;
    AEMtx33Scale(&world, Camera::scale, Camera::scale);
    AEGfxSetTransform(world.m);

    for (auto& hb : enemyHitboxes)
    {
        // draw sprite if present
        if (hb.sprite)
        {
            AEMtx33 m;
            const float spellScale = 5.0f;
            AEMtx33Scale(&m, hb.faceRight ? spellScale : -spellScale, spellScale);
            AEMtx33TransApply(
                &m, &m,
                hb.position.x - (0.5f - hb.sprite->metadata.pivot.x),
                hb.position.y - (- hb.sprite->metadata.pivot.y)
            );
            AEMtx33ScaleApply(&m, &m, Camera::scale, Camera::scale);
            AEGfxSetTransform(m.m);

            hb.sprite->Render();

            // restore world transform before drawing debug rect
            AEGfxSetTransform(world.m);
        }
        // debug rect
        if (debug)
        {
            QuickGraphics::DrawRect(
                hb.position.x, hb.position.y,
                hb.size.x, hb.size.y,
                0xFFFFFF00, // yellow
                AE_GFX_MDM_LINES_STRIP
            );

            // optional center marker
            QuickGraphics::DrawRect(
                hb.position.x, hb.position.y,
                0.05f, 0.05f,
                0xFFFF0000,
                AE_GFX_MDM_LINES_STRIP
            );
        }
    }
}