#pragma once
#pragma once

#include "RoomManager.h"

struct LevelData;

// Builds a room graph from a single full level layout.
// The level is sliced into ROOM_COLS x ROOM_ROWS chunks.
// outStartRoom is set to the room that contains lvl.spawn.
void BuildRoomsFromLevelData(const LevelData& lvl, RoomManager& roomMgr, RoomID& outStartRoom);