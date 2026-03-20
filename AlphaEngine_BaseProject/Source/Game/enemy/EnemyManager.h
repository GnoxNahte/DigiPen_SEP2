#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <algorithm>
#include "Enemy.h"     
#include "Enemyboss.h"
#include "IDamageable.h"
#include "../Rooms/RoomData.h"

enum class EnemySpawnType
{
    Druid = 0,
    Skeleton = 1,
    Boss = 2
};

class EnemyBoss; 

class EnemyManager 
{
public:
    struct SpawnInfo
    {
       // Enemy::Preset preset;
        EnemySpawnType type;
        AEVec2 pos;
    };

public:

 


    void SetBoss(EnemyBoss* b) {
      
        bossDamageable = b;
        boss = b;
       
    }


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

       /* for (const auto& s : spawns)
            enemies.emplace_back(std::make_unique<Enemy>(s.preset, s.pos.x, s.pos.y));*/

        for (const auto& s : spawns)
        switch (s.type)
        {
        case EnemySpawnType::Druid:
        {
            auto e = std::make_unique<Enemy>(Enemy::Preset::Druid, s.pos.x, s.pos.y);
            //DEPTH IS USE TO SCALE THE HEALTH AND DAMAGE OR REGULAR ENEMY
            int depth = 0;
            if (currentRoomId != ROOM_NONE)
                depth = static_cast<int>(currentRoomId) - static_cast<int>(ROOM_1);

            e->ApplyRoomScaling(depth * 10, depth * 1);
            enemies.emplace_back(std::move(e));
        }
        break;

        case EnemySpawnType::Skeleton:
        {
            auto e = std::make_unique<Enemy>(Enemy::Preset::Skeleton, s.pos.x, s.pos.y);
            
            int depth = 0;
            if (currentRoomId != ROOM_NONE)
                depth = static_cast<int>(currentRoomId) - static_cast<int>(ROOM_1);

            e->ApplyRoomScaling(depth * 10, depth * 1);
            enemies.emplace_back(std::move(e));
        }
        break;
    
        case EnemySpawnType::Boss:
            if (boss)
            {
                boss->SetSpawnPosition(s.pos);
            }
            break;
        }
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
    void UpdateAll(const AEVec2& playerpos, bool playerFacingRight, MapGrid& map)
    {
        // update regular enemies
        UpdateAll(playerpos, map);

        // update boss (if present)
        if (boss)
            boss->Update(playerpos, playerFacingRight, map); // or whatever your Player exposes
    }

    void UpdateAll(const AEVec2& playerPos, MapGrid& map)
    {
        for (auto& e : enemies)
            e->Update(playerPos, map);
    }

    void ResetAll()
    {
        enemies.clear();
        enemies.reserve(spawns.size());

        for (const auto& s : spawns)
        {
            switch (s.type)
            {
            case EnemySpawnType::Druid:
            {
                auto e = std::make_unique<Enemy>(Enemy::Preset::Druid, s.pos.x, s.pos.y);
                //DEPTH IS USE TO SCALE THE HEALTH AND DAMAGE OR REGULAR ENEMY
                int depth = 0;
                if (currentRoomId != ROOM_NONE)
                    depth = static_cast<int>(currentRoomId) - static_cast<int>(ROOM_1);

                e->ApplyRoomScaling(depth * 10, depth * 1);
                enemies.emplace_back(std::move(e));
            }
            break;

            case EnemySpawnType::Skeleton:
            {
                auto e = std::make_unique<Enemy>(Enemy::Preset::Skeleton, s.pos.x, s.pos.y);

                int depth = 0;
                if (currentRoomId != ROOM_NONE)
                    depth = static_cast<int>(currentRoomId) - static_cast<int>(ROOM_1);

                e->ApplyRoomScaling(depth * 10, depth * 1);
                enemies.emplace_back(std::move(e));
            }
            break;

            case EnemySpawnType::Boss:
                if (boss)
                    boss->Reset(s.pos);
                break;
            }
        }
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

    void SetCurrentRoomID(RoomID id)
    {
        currentRoomId = id;
    }



private:
    std::vector<SpawnInfo> spawns;                     // editor/level data
    std::vector<std::unique_ptr<Enemy>> enemies;       // runtime instances
    IDamageable* bossDamageable = nullptr;
	EnemyBoss* boss = nullptr; // optional direct pointer if you need boss-specific logic
    
    RoomID currentRoomId = ROOM_1;
    /*
    Enemy::Config BuildScaledConfig(Enemy::Preset preset) const
    {
        Enemy::Config cfg = Enemy::MakePreset(preset);

        int depth = 0;
        if (currentRoomId != ROOM_NONE)
            depth = static_cast<int>(currentRoomId) - static_cast<int>(ROOM_1);

        cfg.maxHp += depth * 10;          // example: +10 hp per deeper room
        cfg.attackDamage += depth * 1;    // example: +1 damage per deeper room

        if (cfg.maxHp < 1) cfg.maxHp = 1;
        if (cfg.attackDamage < 1) cfg.attackDamage = 1;

        return cfg;
    }
    */
};
