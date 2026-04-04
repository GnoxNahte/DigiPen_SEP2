/*!
@file   RoomSystem.h
@author lim kang ping
@brief  This file declares the RoomSystem class.
It controls room loading, room transitions, runtime room objects, and room boundary logic
during gameplay.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
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
    void Update(float dt);

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
    void ApplyStartRoomLeftBoundaryLock();

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
    bool wallSpawnPending = false;     // NEW
    float wallTimer = 0.0f;            // NEW
    static constexpr float kWallSpawnDelay = 0.25f; // NEW
};