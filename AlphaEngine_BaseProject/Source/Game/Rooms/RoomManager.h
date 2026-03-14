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
