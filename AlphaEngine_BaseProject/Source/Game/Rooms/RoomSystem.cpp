#pragma once

#include "RoomSystem.h"

#include "../UI.h"
#include <algorithm>

RoomSystem::RoomSystem(
    MapGrid& mapRef,
    Player& playerRef,
    Camera& cameraRef,
    TrapManager& trapMgrRef,
    EnemyManager& enemyMgrRef,
    EnemyBoss& enemyBossRef,
    RoomManager& roomMgrRef)
    : map(mapRef),
    player(playerRef),
    camera(cameraRef),
    trapMgr(trapMgrRef),
    enemyMgr(enemyMgrRef),
    enemyBoss(enemyBossRef),
    roomMgr(roomMgrRef)
{
}


void RoomSystem::BuildCurrentRoom(RoomDirection cameFrom, const AEVec2* forcedSpawn)
{
    if (roomMgr.GetCurrentRoomID() == ROOM_NONE)
        return;

    const RoomData& room = roomMgr.GetCurrentRoom();
    const AEVec2 roomOrigin = GetRoomOrigin(room.id);

    // Do NOT rebuild the full level map here.
    // GameScene::Init() already rebuilds the full map once.

    ClearRuntimeRoomObjects();

    struct PendingPlateBinding
    {
        PressurePlate* plate = nullptr;
        const RoomTrapSpawn* def = nullptr;
    };

    std::vector<PendingPlateBinding> pendingPlates;
    std::vector<std::pair<int, Trap*>> spawnedById;
    std::vector<Trap*> spawnedSpikes;
    bool hasExplicitLinks = false;

    for (int i = 0; i < room.trapCount; ++i)
    {
        const RoomTrapSpawn& td = room.traps[i];

        Box box{};
        box.size = td.size;
        box.position = AEVec2{
            roomOrigin.x + td.pos.x - td.size.x * 0.5f,
            roomOrigin.y + td.pos.y - td.size.y * 0.5f
        };

        Trap* spawnedTrap = nullptr;
        const Trap::Type tt = static_cast<Trap::Type>(td.type);

        if (tt == Trap::Type::SpikePlate)
        {
            SpikePlate& spikeRef =
                trapMgr.Spawn<SpikePlate>(box, td.upTime, td.downTime, td.damageOnHit, td.startDisabled);

            spawnedTrap = &spikeRef;
            spawnedSpikes.push_back(&spikeRef);
        }
        else if (tt == Trap::Type::PressurePlate)
        {
            PressurePlate& plateRef = trapMgr.Spawn<PressurePlate>(box);
            spawnedTrap = &plateRef;

            pendingPlates.push_back({ &plateRef, &td });
            if (!td.links.empty())
                hasExplicitLinks = true;
        }
        else if (tt == Trap::Type::LavaPool)
        {
            LavaPool& lavaRef = trapMgr.Spawn<LavaPool>(box, td.damagePerTick, td.tickInterval);
            spawnedTrap = &lavaRef;
        }

        if (spawnedTrap && td.id >= 0)
            spawnedById.push_back({ td.id, spawnedTrap });
    }

    auto FindSpawnedTrapById = [&spawnedById](int id) -> Trap*
        {
            for (const auto& entry : spawnedById)
            {
                if (entry.first == id)
                    return entry.second;
            }
            return nullptr;
        };

    if (hasExplicitLinks)
    {
        for (const auto& pending : pendingPlates)
        {
            if (!pending.plate || !pending.def)
                continue;

            for (int linkId : pending.def->links)
            {
                Trap* target = FindSpawnedTrapById(linkId);
                if (target)
                    pending.plate->AddLinkedTrap(target);
            }
        }
    }
    else
    {
        for (const auto& pending : pendingPlates)
        {
            if (!pending.plate)
                continue;

            for (Trap* spike : spawnedSpikes)
            {
                if (spike)
                    pending.plate->AddLinkedTrap(spike);
            }
        }
    }

    std::vector<EnemyManager::SpawnInfo> spawns;
    bool hasBoss = false;

    for (int i = 0; i < room.enemyCount; ++i)
    {
        EnemySpawnType type = static_cast<EnemySpawnType>(room.enemies[i].preset);

        AEVec2 worldPos{
            roomOrigin.x + room.enemies[i].pos.x,
            roomOrigin.y + room.enemies[i].pos.y
        };

        spawns.push_back({ type, worldPos });

        if (type == EnemySpawnType::Boss)
            hasBoss = true;
    }

    activeBoss = hasBoss ? &enemyBoss : nullptr;

    enemyMgr.SetBoss(activeBoss);
    enemyMgr.SetCurrentRoomID(room.id);
    enemyMgr.SetSpawns(spawns);
    enemyMgr.SpawnAll();

    AEVec2 spawn{
        roomOrigin.x + room.startSpawn.x,
        roomOrigin.y + room.startSpawn.y
    };

    if (forcedSpawn)
    {
        spawn = *forcedSpawn;
    }
    else if (cameFrom != DIR_NONE)
    {
        const AEVec2 roomMin{
            roomOrigin.x + 0.35f,
            roomOrigin.y + 0.35f
        };
        const AEVec2 roomMax{
            roomOrigin.x + static_cast<float>(ROOM_COLS) - 0.35f,
            roomOrigin.y + static_cast<float>(ROOM_ROWS) - 0.35f
        };

        spawn = AEVec2{
            std::clamp(spawn.x, roomMin.x, roomMax.x),
            std::clamp(spawn.y, roomMin.y, roomMax.y)
        };
    }

    // First entry into the scene = full reset.
    // Room-to-room transition = just reposition.
   /* if (cameFrom == DIR_NONE && forcedSpawn == nullptr)
        player.Reset(spawn);
    else
        player.SetPosition(spawn);
        */

    const bool snapCamera = (cameFrom == DIR_NONE);
    camera.SetFollow(&player.GetPosition(), 0.f, 0.f, snapCamera);
    if (snapCamera)
        camera.Update();

    ApplyBlockedReturnBarrier();

    if (roomMgr.GetCurrentRoomID() == ROOM_11)
        UI::StartBossIntro();
}
void RoomSystem::ClearRuntimeRoomObjects()
{
    trapMgr = TrapManager{};

    enemyMgr = EnemyManager{};
    activeBoss = nullptr;
    enemyMgr.SetBoss(nullptr);
}

RoomDirection RoomSystem::CheckRoomExit() const
{
    if (roomMgr.GetCurrentRoomID() == ROOM_NONE)
        return DIR_NONE;

    const AEVec2 p = player.GetPosition();
    const AEVec2 origin = GetRoomOrigin(roomMgr.GetCurrentRoomID());

    if (p.y < origin.y)
        return DIR_BOTTOM;
    if (p.y >= origin.y + static_cast<float>(ROOM_ROWS))
        return DIR_TOP;
    if (p.x < origin.x)
        return DIR_LEFT;
    if (p.x >= origin.x + static_cast<float>(ROOM_COLS))
        return DIR_RIGHT;

    return DIR_NONE;
}

AEVec2 RoomSystem::GetRoomOrigin(RoomID id) const
{
    if (id == ROOM_NONE || !roomMgr.HasRoom(id))
        return AEVec2{ 0.f, 0.f };

    const RoomData& room = roomMgr.GetRoom(id);

    return AEVec2{
        room.gridX * static_cast<float>(ROOM_COLS),
        room.gridY * static_cast<float>(ROOM_ROWS)
    };
}

AEVec2 RoomSystem::ComputeTransitionSpawn(
    RoomID previousRoom,
    RoomID nextRoom,
    const AEVec2& previousPos) const
{
    static constexpr float kInset = 0.35f;

    const AEVec2 prevOrigin = GetRoomOrigin(previousRoom);
    const AEVec2 nextOrigin = GetRoomOrigin(nextRoom);

    AEVec2 spawn = previousPos;

    auto ClampFloat = [](float v, float lo, float hi) -> float
        {
            return (v < lo) ? lo : ((v > hi) ? hi : v);
        };

    const float nextMinX = nextOrigin.x + kInset;
    const float nextMaxX = nextOrigin.x + static_cast<float>(ROOM_COLS) - kInset;
    const float nextMinY = nextOrigin.y + kInset;
    const float nextMaxY = nextOrigin.y + static_cast<float>(ROOM_ROWS) - kInset;

    if (nextOrigin.x > prevOrigin.x)
    {
        spawn.x = nextMinX;
        spawn.y = ClampFloat(previousPos.y, nextMinY, nextMaxY);
    }
    else if (nextOrigin.x < prevOrigin.x)
    {
        spawn.x = nextMaxX;
        spawn.y = ClampFloat(previousPos.y, nextMinY, nextMaxY);
    }
    else if (nextOrigin.y > prevOrigin.y)
    {
        spawn.x = ClampFloat(previousPos.x, nextMinX, nextMaxX);
        spawn.y = nextMinY;
    }
    else if (nextOrigin.y < prevOrigin.y)
    {
        spawn.x = ClampFloat(previousPos.x, nextMinX, nextMaxX);
        spawn.y = nextMaxY;
    }
    else
    {
        spawn.x = ClampFloat(previousPos.x, nextMinX, nextMaxX);
        spawn.y = ClampFloat(previousPos.y, nextMinY, nextMaxY);
    }

    return spawn;
}

void RoomSystem::SetBlockedReturnDir(RoomDirection dir)
{
    blockedReturnDir = dir;
}

RoomDirection RoomSystem::GetBlockedReturnDir() const
{
    return blockedReturnDir;
}

void RoomSystem::ClearBlockedReturnDir()
{
    blockedReturnDir = DIR_NONE;
}

EnemyBoss* RoomSystem::GetActiveBoss()
{
    return activeBoss;
}

const EnemyBoss* RoomSystem::GetActiveBoss() const
{
    return activeBoss;
}

void RoomSystem::ApplyBlockedReturnBarrier()
{
    if (blockedReturnDir == DIR_NONE || roomMgr.GetCurrentRoomID() == ROOM_NONE)
        return;

    const AEVec2 origin = GetRoomOrigin(roomMgr.GetCurrentRoomID());
    const int ox = static_cast<int>(origin.x);
    const int oy = static_cast<int>(origin.y);

    const MapTile::Type kBlockTile = MapTile::Type::GROUND_BODY;

    switch (blockedReturnDir)
    {
    case DIR_BOTTOM:
        for (int x = 0; x < ROOM_COLS; ++x)
            map.SetTile(ox + x, oy - 1, kBlockTile);
        break;

    case DIR_TOP:
        for (int x = 0; x < ROOM_COLS; ++x)
            map.SetTile(ox + x, oy + ROOM_ROWS, kBlockTile);
        break;

    case DIR_LEFT:
        for (int y = 0; y < ROOM_ROWS; ++y)
            map.SetTile(ox - 1, oy + y, kBlockTile);
        break;

    case DIR_RIGHT:
        for (int y = 0; y < ROOM_ROWS; ++y)
            map.SetTile(ox + ROOM_COLS, oy + y, kBlockTile);
        break;

    default:
        break;
    }
}