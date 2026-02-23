#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <algorithm>

#include "AEEngine.h"    
#include "Enemy.h"   
#include "IDamageable.h"

class EnemyManager 
{
public:
    struct SpawnInfo
    {
        Enemy::Preset preset;
        AEVec2 pos;
    };

public:


    void SetBoss(IDamageable* b) { bossDamageable = b; }


    EnemyManager() = default;

    // --- Data-driven spawning (use this for editor / level load) ---
    void SetSpawns(const std::vector<SpawnInfo>& newSpawns)
    {
        spawns = newSpawns;
    }

    void ClearSpawns()
    {
        spawns.clear();
    }

    void SpawnAll()
    {
        enemies.clear();
        enemies.reserve(spawns.size());

        for (const auto& s : spawns)
            enemies.emplace_back(std::make_unique<Enemy>(s.preset, s.pos.x, s.pos.y));
    }

    // --- Manual spawn (optional) ---
    Enemy& Spawn(Enemy::Preset preset, const AEVec2& pos)
    {
        enemies.emplace_back(std::make_unique<Enemy>(preset, pos.x, pos.y));
        return *enemies.back();
    }

    // --- Despawn ---
    void DespawnAll()
    {
        enemies.clear();
    }

    // Optional helper: remove enemies that match a predicate
    template<typename Pred>
    void DespawnWhere(Pred&& pred)
    {
        enemies.erase(
            std::remove_if(enemies.begin(), enemies.end(),
                [&](const std::unique_ptr<Enemy>& e) { return pred(*e); }),
            enemies.end());
    }

    // --- Update/Render ---
    void UpdateAll(const AEVec2& playerPos)
    {
        for (auto& e : enemies)
            e->Update(playerPos);
    }

    void RenderAll()
    {
        for (auto& e : enemies)
            e->Render();
    }

    // --- Collision / queries ---
    template<typename Fn>
    void ForEachEnemy(Fn&& fn)
    {
        for (auto& e : enemies)
            fn(*e);
    }

    template<typename Fn>
    void ForEachDamageable(Fn&& fn)
    {
        for (auto& e : enemies) fn(static_cast<IDamageable&>(*e));
        if (bossDamageable) fn(*bossDamageable);
    }

    int Count() const { return (int)enemies.size(); }

private:
    std::vector<SpawnInfo> spawns;                     // editor/level data
    std::vector<std::unique_ptr<Enemy>> enemies;       // runtime instances
    IDamageable* bossDamageable = nullptr;
};
