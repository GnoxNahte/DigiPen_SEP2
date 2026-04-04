/*!
@file   RoomManager.h
@author lim kang ping, Santosh
@brief  This file declares the RoomManager class.
It manages room data, keeps track of the current room, and provides functions
for room access, room changes, and entry spawn positions.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
#pragma once

#include "RoomData.h"

class RoomManager
{
public:
	RoomManager();

	void Clear();

	bool HasRoom(RoomID id) const;
	void SetRoom(RoomID id, const RoomData& data);
	RoomData& GetRoom(RoomID id);
	const RoomData& GetRoom(RoomID id) const;

	void SetCurrentRoom(RoomID id);
	RoomID GetCurrentRoomID() const;

	RoomData& GetCurrentRoom();
	const RoomData& GetCurrentRoom() const;

	RoomID GetNeighbor(RoomID id, RoomDirection dir) const;
	bool ChangeRoom(RoomDirection dir);

	AEVec2 GetEntrySpawn(RoomID room, RoomDirection cameFrom) const;

private:
	RoomData rooms[ROOM_COUNT];
	bool roomUsed[ROOM_COUNT];
	RoomID currentRoom;
};
