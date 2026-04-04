/*!
@file   RoomBuilder.h
@author lim kang ping
@brief  This file declares the function used to build rooms from level data.
It is used to create room information and determine the starting room from a full level.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
#pragma once

#include "RoomManager.h"

struct LevelData;

// Builds a room graph from a single full level layout.
// The level is sliced into ROOM_COLS x ROOM_ROWS chunks.
// outStartRoom is set to the room that contains lvl.spawn.
void BuildRoomsFromLevelData(const LevelData& lvl, RoomManager& roomMgr, RoomID& outStartRoom);