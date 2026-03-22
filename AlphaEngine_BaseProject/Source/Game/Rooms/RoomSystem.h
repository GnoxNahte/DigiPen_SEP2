#pragma once
#pragma once

#include "../Rooms/RoomManager.h"
#include "../Environment/MapGrid.h"
#include "../Environment/traps.h"
#include "../Player/Player.h"
#include "../Camera.h"
#include "../enemy/EnemyManager.h"
#include "../enemy/EnemyBoss.h"

class RoomSystem
{
public:
    RoomSystem(
        MapGrid& map,
        Player& player,
        Camera& camera,
        TrapManager& trapMgr,
        EnemyManager& enemyMgr,
        EnemyBoss& enemyBoss,
        RoomManager& roomMgr
    );

    void BuildCurrentRoom(RoomDirection cameFrom = DIR_NONE,
        const AEVec2* forcedSpawn = nullptr);

    void ClearRuntimeRoomObjects();

    RoomDirection CheckRoomExit() const;
    AEVec2 GetRoomOrigin(RoomID id) const;
    AEVec2 ComputeTransitionSpawn(RoomID previousRoom,
        RoomID nextRoom,
        const AEVec2& previousPos) const;

    void SetBlockedReturnDir(RoomDirection dir);
    RoomDirection GetBlockedReturnDir() const;
    void ClearBlockedReturnDir();

    EnemyBoss* GetActiveBoss();
    const EnemyBoss* GetActiveBoss() const;

private:
    void ApplyBlockedReturnBarrier();

private:
    MapGrid& map;
    Player& player;
    Camera& camera;
    TrapManager& trapMgr;
    EnemyManager& enemyMgr;
    EnemyBoss& enemyBoss;
    RoomManager& roomMgr;

    EnemyBoss* activeBoss = nullptr;
    RoomDirection blockedReturnDir = DIR_NONE;
};