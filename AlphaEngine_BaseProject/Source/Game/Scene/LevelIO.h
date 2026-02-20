#pragma once
#include <vector>
#include <string>
#include "AEEngine.h"

class MapGrid;

struct TrapDefSimple
{
    int type = 0;    // Trap::Type stored as int
    AEVec2 pos{ 0,0 };
    AEVec2 size{ 1,1 };

    // spike params
    float upTime = 1.f;
    float downTime = 1.f;
    int damageOnHit = 10;
    bool startDisabled = false;

    // lava params
    int damagePerTick = 1;
    float tickInterval = 0.2f;

    // plate params (optional)
    int id = -1;
    std::vector<int> links;
};

struct LevelData
{
    int rows = 0;
    int cols = 0;

    AEVec2 spawn{ 5.f, 5.f };
    std::vector<int> tiles;                 // row-major
    std::vector<TrapDefSimple> traps;
};

bool SaveLevelToFile(const char* filename, const LevelData& lvl);
bool LoadLevelFromFile(const char* filename, LevelData& out);

// NOTE: MapGrid::GetTile is often NON-const in your codebase,
// so we take MapGrid& (not const MapGrid&).
void BuildLevelDataFromEditor(MapGrid& grid, int rows, int cols,
    const std::vector<TrapDefSimple>& traps,
    const AEVec2& spawn,
    LevelData& out);

bool ApplyLevelDataToEditor(const LevelData& lvl,
    MapGrid*& ioGrid,
    std::vector<TrapDefSimple>& ioTraps,
    AEVec2& ioSpawn);
