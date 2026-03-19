#include "RoomManager.h"
#include <cassert>

RoomManager::RoomManager()
{
	Clear();
}

void RoomManager::Clear()
{
	for (int i = 0; i < ROOM_COUNT; ++i)
	{
		roomUsed[i] = false;
		rooms[i] = RoomData{};
	}
	currentRoom = ROOM_NONE;
}

bool RoomManager::HasRoom(RoomID id) const
{
	if (id < ROOM_1 || (int)id >= ROOM_COUNT)
		return false;

	return roomUsed[(int)id];
}

void RoomManager::SetRoom(RoomID id, const RoomData& data)
{
	if (id < ROOM_1 || (int)id >= ROOM_COUNT)
		return;

	rooms[(int)id] = data;
	rooms[(int)id].id = id;
	roomUsed[(int)id] = true;
}

RoomData& RoomManager::GetRoom(RoomID id)
{
	assert(id >= ROOM_1 && (int)id < ROOM_COUNT);
	return rooms[(int)id];
}

const RoomData& RoomManager::GetRoom(RoomID id) const
{
	assert(id >= ROOM_1 && (int)id < ROOM_COUNT);
	return rooms[(int)id];
}

void RoomManager::SetCurrentRoom(RoomID id)
{
	if (!HasRoom(id))
		return;

	currentRoom = id;
}

RoomID RoomManager::GetCurrentRoomID() const
{
	return currentRoom;
}

RoomData& RoomManager::GetCurrentRoom()
{
	assert(currentRoom != ROOM_NONE);
	return rooms[(int)currentRoom];
}

const RoomData& RoomManager::GetCurrentRoom() const
{
	assert(currentRoom != ROOM_NONE);
	return rooms[(int)currentRoom];
}

RoomID RoomManager::GetNeighbor(RoomID id, RoomDirection dir) const
{
	if (!HasRoom(id))
		return ROOM_NONE;

	const RoomData& room = rooms[(int)id];

	switch (dir)
	{
	case DIR_TOP:    return room.topRoom;
	case DIR_LEFT:   return room.leftRoom;
	case DIR_BOTTOM: return room.bottomRoom;
	case DIR_RIGHT:  return room.rightRoom;
	default:         return ROOM_NONE;
	}
}

bool RoomManager::ChangeRoom(RoomDirection dir)
{
	if (currentRoom == ROOM_NONE)
		return false;

	RoomID next = GetNeighbor(currentRoom, dir);
	if (!HasRoom(next))
		return false;

	currentRoom = next;
	return true;
}

AEVec2 RoomManager::GetEntrySpawn(RoomID room, RoomDirection cameFrom) const
{
	if (!HasRoom(room))
		return AEVec2{ 2.5f, 2.5f };

	const RoomData& r = rooms[(int)room];

	switch (cameFrom)
	{
	case DIR_TOP:    return r.entryFromTop;
	case DIR_LEFT:   return r.entryFromLeft;
	case DIR_BOTTOM: return r.entryFromBottom;
	case DIR_RIGHT:  return r.entryFromRight;
	default:         return r.startSpawn;
	}
}