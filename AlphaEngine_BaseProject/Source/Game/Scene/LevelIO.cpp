#include "LevelIO.h"
#include <fstream>
#include <sstream>

static const char* TrapTypeToString(Trap::Type t)
{
    switch (t)
    {
    case Trap::Type::LavaPool:      return "lavapool";
    case Trap::Type::PressurePlate: return "pressureplate";
    case Trap::Type::SpikePlate:    return "spikeplate";
    default:                        return "unknown";
    }
}

static bool StringToTrapType(const std::string& s, Trap::Type& out)
{
    if (s == "lavapool") { out = Trap::Type::LavaPool; return true; }
    if (s == "pressureplate") { out = Trap::Type::PressurePlate; return true; }
    if (s == "spikeplate") { out = Trap::Type::SpikePlate; return true; }
    return false;
}

bool SaveLevelToFile(const char* filename, const LevelData& lvl)
{
    std::ofstream out(filename);
    if (!out) return false;

    out << "version 1\n";
    out << "size " << lvl.rows << " " << lvl.cols << "\n";
    out << "spawn " << (int)lvl.spawn.x << " " << (int)lvl.spawn.y << "\n";

    out << "tiles\n";
    // tiles stored row-major: [y*cols + x]
    for (int y = 0; y < lvl.rows; ++y)
    {
        for (int x = 0; x < lvl.cols; ++x)
        {
            int v = lvl.tiles[y * lvl.cols + x];
            out << v;
            if (x + 1 < lvl.cols) out << ' ';
        }
        out << '\n';
    }

    out << "traps " << (int)lvl.traps.size() << "\n";
    for (const auto& t : lvl.traps)
    {
        out << TrapTypeToString(t.type) << ' ';
        out << t.box.position.x << ' ' << t.box.position.y << ' ';
        out << t.box.size.x << ' ' << t.box.size.y << ' ';

        if (t.type == Trap::Type::LavaPool)
        {
            out << "dpt " << t.damagePerTick << " tick " << t.tickInterval;
        }
        else if (t.type == Trap::Type::SpikePlate)
        {
            out << "up " << t.upTime << " down " << t.downTime
                << " dmg " << t.damageOnHit
                << " startDisabled " << (t.startDisabled ? 1 : 0);
        }
        else if (t.type == Trap::Type::PressurePlate)
        {
            out << "id " << t.id << " links " << (int)t.links.size();
            for (int id : t.links) out << ' ' << id;
        }

        out << '\n';
    }

    return true;
}

bool LoadLevelFromFile(const char* filename, LevelData& outLvl)
{
    std::ifstream in(filename);
    if (!in) return false;

    std::string word;
    int version = 0;

    in >> word >> version; // version N
    if (!in || word != "version") return false;

    in >> word >> outLvl.rows >> outLvl.cols; // size r c
    if (!in || word != "size") return false;

    in >> word >> outLvl.spawn.x >> outLvl.spawn.y; // spawn x y
    if (!in || word != "spawn") return false;

    in >> word; // tiles
    if (!in || word != "tiles") return false;

    outLvl.tiles.assign(outLvl.rows * outLvl.cols, 0);

    for (int y = 0; y < outLvl.rows; ++y)
    {
        for (int x = 0; x < outLvl.cols; ++x)
        {
            int v = 0;
            in >> v;
            if (!in) return false;
            outLvl.tiles[y * outLvl.cols + x] = v;
        }
    }

    in >> word; // traps
    if (!in || word != "traps") return false;

    int trapCount = 0;
    in >> trapCount;
    if (!in || trapCount < 0) return false;

    outLvl.traps.clear();
    outLvl.traps.reserve((size_t)trapCount);

    // consume endline
    std::string line;
    std::getline(in, line);

    for (int i = 0; i < trapCount; ++i)
    {
        std::getline(in, line);
        if (!in) return false;

        std::istringstream ss(line);

        std::string typeStr;
        TrapDef t{};

        ss >> typeStr;
        if (!StringToTrapType(typeStr, t.type)) return false;

        ss >> t.box.position.x >> t.box.position.y >> t.box.size.x >> t.box.size.y;
        if (!ss) return false;

        // parse key-value params
        while (ss)
        {
            std::string key;
            ss >> key;
            if (!ss) break;

            if (t.type == Trap::Type::LavaPool)
            {
                if (key == "dpt") ss >> t.damagePerTick;
                else if (key == "tick") ss >> t.tickInterval;
                else { /* ignore unknown */ }
            }
            else if (t.type == Trap::Type::SpikePlate)
            {
                if (key == "up") ss >> t.upTime;
                else if (key == "down") ss >> t.downTime;
                else if (key == "dmg") ss >> t.damageOnHit;
                else if (key == "startDisabled")
                {
                    int v = 0; ss >> v;
                    t.startDisabled = (v != 0);
                }
                else { /* ignore */ }
            }
            else if (t.type == Trap::Type::PressurePlate)
            {
                if (key == "id") ss >> t.id;
                else if (key == "links")
                {
                    int n = 0; ss >> n;
                    t.links.clear();
                    for (int k = 0; k < n; ++k)
                    {
                        int id = -1; ss >> id;
                        t.links.push_back(id);
                    }
                }
                else { /* ignore */ }
            }
        }

        outLvl.traps.push_back(t);
    }

    return true;
}

void BuildLevelDataFromEditor(const MapGrid& grid, int rows, int cols,
    const std::vector<TrapDef>& traps,
    const AEVec2& spawn,
    LevelData& out)
{
    out.rows = rows;
    out.cols = cols;
    out.spawn = spawn;
    out.tiles.assign(rows * cols, 0);
    out.traps = traps;

    for (int y = 0; y < rows; ++y)
    {
        for (int x = 0; x < cols; ++x)
        {
            const MapTile* t = grid.GetTile(x, y);
            out.tiles[y * cols + x] = t ? (int)t->type : 0;
        }
    }
}

bool ApplyLevelDataToEditor(const LevelData& lvl,
    MapGrid*& ioGrid,
    std::vector<TrapDef>& ioTraps,
    AEVec2& ioSpawn)
{
    if (lvl.rows <= 0 || lvl.cols <= 0) return false;
    if ((int)lvl.tiles.size() != lvl.rows * lvl.cols) return false;

    delete ioGrid;
    ioGrid = new MapGrid(lvl.rows, lvl.cols);

    for (int y = 0; y < lvl.rows; ++y)
    {
        for (int x = 0; x < lvl.cols; ++x)
        {
            int v = lvl.tiles[y * lvl.cols + x];
            if (v < 0) v = 0;
            if (v >= MapTile::typeCount) v = 0;
            ioGrid->SetTile(x, y, (MapTile::Type)v);
        }
    }

    ioTraps = lvl.traps;
    ioSpawn = lvl.spawn;
    return true;
}
