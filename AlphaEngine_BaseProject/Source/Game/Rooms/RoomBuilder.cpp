#include "RoomBuilder.h"

#include "../Scene/LevelIO.h"
#include <algorithm>

namespace
{


    RoomID RoomIdFromGrid(int rx, int ry)
    {
        if (rx == 0 && ry == 0) return ROOM_1;
        if (rx == 1 && ry == 0) return ROOM_2;
        if (rx == 2 && ry == 0) return ROOM_3;
        if (rx == 2 && ry == 1) return ROOM_4;
        if (rx == 1 && ry == 1) return ROOM_5;
        if (rx == 0 && ry == 1) return ROOM_6;
        if (rx == 0 && ry == 2) return ROOM_7;
        if (rx == 1 && ry == 2) return ROOM_8;
        if (rx == 2 && ry == 2) return ROOM_9;
        if (rx == 3 && ry == 2) return ROOM_10;
        if (rx == 3 && ry == 1) return ROOM_11;

        return ROOM_NONE;
    }

    bool PointInRoom(const AEVec2& p, int roomX, int roomY)
    {
        const float minX = static_cast<float>(roomX * ROOM_COLS);
        const float minY = static_cast<float>(roomY * ROOM_ROWS);
        const float maxX = minX + static_cast<float>(ROOM_COLS);
        const float maxY = minY + static_cast<float>(ROOM_ROWS);

        return p.x >= minX && p.x < maxX &&
            p.y >= minY && p.y < maxY;
    }
}

void BuildRoomsFromLevelData(const LevelData& lvl, RoomManager& roomMgr, RoomID& outStartRoom)
{
    roomMgr.Clear();
    outStartRoom = ROOM_1;

    if (lvl.cols <= 0 || lvl.rows <= 0)
        return;

    const int roomsX = lvl.cols / ROOM_COLS;
    const int roomsY = lvl.rows / ROOM_ROWS;

    if (roomsX <= 0 || roomsY <= 0)
        return;

    const int totalRooms = roomsX * roomsY;
    const int roomCountToUse = (std::min)(totalRooms, ROOM_COUNT);

    for (int i = 0; i < roomCountToUse; ++i)
    {
        const int rx = i % roomsX;
        const int ry = i / roomsX;

        RoomData room{};
        room.id = RoomIdFromGrid(rx, ry);

        if (room.id == ROOM_NONE)
            continue;

        room.gridX = rx;
        room.gridY = ry;

        room.topRoom = RoomIdFromGrid(rx, ry + 1);
        room.leftRoom = RoomIdFromGrid(rx - 1, ry);
        room.bottomRoom = RoomIdFromGrid(rx, ry - 1);
        room.rightRoom = RoomIdFromGrid(rx + 1, ry);

        // Tiles
        for (int y = 0; y < ROOM_ROWS; ++y)
        {
            for (int x = 0; x < ROOM_COLS; ++x)
            {
                const int globalX = rx * ROOM_COLS + x;
                const int globalY = ry * ROOM_ROWS + y;

                int v = static_cast<int>(MapTile::Type::NONE);
                if (globalX >= 0 && globalX < lvl.cols &&
                    globalY >= 0 && globalY < lvl.rows)
                {
                    v = lvl.tiles[static_cast<size_t>(globalY) * static_cast<size_t>(lvl.cols) +
                        static_cast<size_t>(globalX)];
                }

                if (v < 0 || v >= MapTile::typeCount)
                    v = static_cast<int>(MapTile::Type::NONE);

                room.tiles[y][x] = static_cast<MapTile::Type>(v);
            }
        }

        // Default entry points
        room.entryFromLeft = { 3.0f, 3.0f };
        room.entryFromRight = { static_cast<float>(ROOM_COLS) - 4.0f, 3.0f };
        room.entryFromTop = { ROOM_COLS * 0.5f, static_cast<float>(ROOM_ROWS) - 4.0f };
        room.entryFromBottom = { ROOM_COLS * 0.5f, 3.0f };

        // Default spawn
        room.startSpawn = { 2.5f, 3.0f };

        // Player start room
        if (PointInRoom(lvl.spawn, rx, ry))
        {
            room.startSpawn = {
                lvl.spawn.x - rx * static_cast<float>(ROOM_COLS),
                lvl.spawn.y - ry * static_cast<float>(ROOM_ROWS)
            };
            outStartRoom = room.id;
        }

        // Enemies
        for (const auto& e : lvl.enemies)
        {
            if (!PointInRoom(e.pos, rx, ry))
                continue;

            if (room.enemyCount >= MAX_ROOM_ENEMIES)
                continue;

            room.enemies[room.enemyCount].preset = e.preset;
            room.enemies[room.enemyCount].pos = {
                e.pos.x - rx * static_cast<float>(ROOM_COLS),
                e.pos.y - ry * static_cast<float>(ROOM_ROWS)
            };
            ++room.enemyCount;
        }

        // Traps
        for (const auto& t : lvl.traps)
        {
            if (!PointInRoom(t.pos, rx, ry))
                continue;

            if (room.trapCount >= MAX_ROOM_TRAPS)
                continue;

            RoomTrapSpawn& dst = room.traps[room.trapCount];
            dst.id = t.id;                  // NEW
            dst.type = t.type;
            dst.pos = {
                t.pos.x - rx * static_cast<float>(ROOM_COLS),
                t.pos.y - ry * static_cast<float>(ROOM_ROWS)
            };
            dst.size = t.size;
            dst.upTime = t.upTime;
            dst.downTime = t.downTime;
            dst.damageOnHit = t.damageOnHit;
            dst.startDisabled = t.startDisabled;
            dst.damagePerTick = t.damagePerTick;
            dst.tickInterval = t.tickInterval;
            dst.links = t.links;            // NEW
            ++room.trapCount;
        }

        roomMgr.SetRoom(room.id, room);
    }
}