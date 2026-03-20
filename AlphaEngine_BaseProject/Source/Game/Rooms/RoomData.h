#pragma once

#include "../Environment/MapTile.h"
#include "AEEngine.h"
#include <vector>

static constexpr int ROOM_COLS = 25;
static constexpr int ROOM_ROWS = 14;
static constexpr int MAX_ROOM_ENEMIES = 32;
static constexpr int MAX_ROOM_TRAPS = 32;
static constexpr int ROOM_COUNT = 20;

enum RoomID
{
	ROOM_NONE = -1,
	ROOM_1 = 0,
	ROOM_2,
	ROOM_3,
	ROOM_4,
	ROOM_5,
	ROOM_6,
	ROOM_7,
	ROOM_8,
	ROOM_9,
	ROOM_10,
	ROOM_11,
	ROOM_12,
	ROOM_13,
	ROOM_14,
	ROOM_15,
	ROOM_16,
	ROOM_17,
	ROOM_18,
	ROOM_19, 
	ROOM_20
};

enum RoomDirection
{
	DIR_NONE = 0,
	DIR_TOP,
	DIR_LEFT,
	DIR_BOTTOM,
	DIR_RIGHT
};

struct RoomEnemySpawn
{
	int preset = 0;
	AEVec2 pos{ 0.f, 0.f };
};

struct RoomTrapSpawn
{
	int id = -1;
	int type = 0;
	AEVec2 pos{ 0.f, 0.f };
	AEVec2 size{ 1.f, 1.f };

	float upTime = 1.0f;
	float downTime = 1.0f;
	int damageOnHit = 10;
	bool startDisabled = false;

	int damagePerTick = 1;
	float tickInterval = 0.2f;

	std::vector<int> links;
};

struct RoomData
{
	RoomID id = ROOM_NONE;
	int gridX = 0;
	int gridY = 0;
	MapTile::Type tiles[ROOM_ROWS][ROOM_COLS]{};

	RoomID topRoom = ROOM_NONE;
	RoomID leftRoom = ROOM_NONE;
	RoomID bottomRoom = ROOM_NONE;
	RoomID rightRoom = ROOM_NONE;

	AEVec2 startSpawn{ 2.5f, 2.5f };
	AEVec2 entryFromTop{ 12.5f, 11.5f };
	AEVec2 entryFromLeft{ 1.5f, 7.5f };
	AEVec2 entryFromBottom{ 12.5f, 1.5f };
	AEVec2 entryFromRight{ 23.5f, 7.5f };

	RoomEnemySpawn enemies[MAX_ROOM_ENEMIES]{};
	int enemyCount = 0;

	RoomTrapSpawn traps[MAX_ROOM_TRAPS]{};
	int trapCount = 0;


};
