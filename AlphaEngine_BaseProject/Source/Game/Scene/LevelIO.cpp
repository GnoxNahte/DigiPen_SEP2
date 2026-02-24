#include "LevelIO.h"

#include "../Environment/MapGrid.h"
#include "../Environment/MapTile.h"
#include "../Environment/traps.h" // Trap::Type

#include <fstream>
#include <sstream>
#include <cctype>
#include <filesystem>
#include <system_error>

namespace
{
    const char* TrapTypeToString(int typeAsInt)
    {
        const Trap::Type t = static_cast<Trap::Type>(typeAsInt);
        switch (t)
        {
        case Trap::Type::LavaPool:      return "lavapool";
        case Trap::Type::PressurePlate: return "pressureplate";
        case Trap::Type::SpikePlate:    return "spikeplate";
        default:                        return "unknown";
        }
    }

    bool StringToTrapTypeInt(const std::string& s, int& outTypeAsInt)
    {
        if (s == "lavapool") { outTypeAsInt = (int)Trap::Type::LavaPool; return true; }
        if (s == "pressureplate") { outTypeAsInt = (int)Trap::Type::PressurePlate; return true; }
        if (s == "spikeplate") { outTypeAsInt = (int)Trap::Type::SpikePlate; return true; }
        return false;
    }

    bool ParseTrapType(const std::string& token, int& outTypeAsInt)
    {
        bool isNumber = !token.empty();
        for (char c : token)
        {
            if (!std::isdigit((unsigned char)c) && c != '-' && c != '+') { isNumber = false; break; }
        }

        if (isNumber) { outTypeAsInt = std::stoi(token); return true; }
        return StringToTrapTypeInt(token, outTypeAsInt);
    }

    // match your Enemy::Preset order:
    // 0 = druid, 1 = skeleton
    const char* EnemyPresetToString(int presetAsInt)
    {
        switch (presetAsInt)
        {
        case 0: return "druid";
        case 1: return "skeleton";
        default: return "unknown";
        }
    }

    bool ParseEnemyPreset(const std::string& token, int& outPresetAsInt)
    {
        bool isNumber = !token.empty();
        for (char c : token)
        {
            if (!std::isdigit((unsigned char)c) && c != '-' && c != '+') { isNumber = false; break; }
        }

        if (isNumber) { outPresetAsInt = std::stoi(token); return true; }
        if (token == "druid") { outPresetAsInt = 0; return true; }
        if (token == "skeleton") { outPresetAsInt = 1; return true; }
        return false;
    }

    void TrimLeft(std::string& s)
    {
        size_t i = 0;
        while (i < s.size() && std::isspace((unsigned char)s[i])) ++i;
        s.erase(0, i);
    }
}

bool SaveLevelToFile(const char* filename, const LevelData& lvl)
{
    if (!filename) return false;
    if (lvl.rows <= 0 || lvl.cols <= 0) return false;
    if ((int)lvl.tiles.size() != lvl.rows * lvl.cols) return false;

    // ensure parent directory exists (if using folders)
    {
        std::error_code ec;
        std::filesystem::path p(filename);
        const auto parent = p.parent_path();
        if (!parent.empty())
        {
            std::filesystem::create_directories(parent, ec);
            if (ec) return false;
        }
    }

    std::ofstream out(filename, std::ios::out | std::ios::trunc);
    if (!out.is_open()) return false;

    out << "version 1\n";
    out << "size " << lvl.rows << " " << lvl.cols << "\n";
    out << "spawn " << lvl.spawn.x << " " << lvl.spawn.y << "\n";

    out << "tiles\n";
    for (int y = 0; y < lvl.rows; ++y)
    {
        for (int x = 0; x < lvl.cols; ++x)
        {
            const int v = lvl.tiles[y * lvl.cols + x];
            out << v;
            if (x + 1 < lvl.cols) out << ' ';
        }
        out << "\n";
    }

    out << "traps " << (int)lvl.traps.size() << "\n";
    for (const auto& t : lvl.traps)
    {
        out << "trap " << TrapTypeToString(t.type) << ' ';
        out << t.pos.x << ' ' << t.pos.y << ' ';
        out << t.size.x << ' ' << t.size.y;

        const Trap::Type tt = static_cast<Trap::Type>(t.type);
        if (tt == Trap::Type::SpikePlate)
        {
            out << " up " << t.upTime
                << " down " << t.downTime
                << " dmg " << t.damageOnHit
                << " startDisabled " << (t.startDisabled ? 1 : 0);
        }
        else if (tt == Trap::Type::LavaPool)
        {
            out << " dpt " << t.damagePerTick
                << " tick " << t.tickInterval;
        }
        else if (tt == Trap::Type::PressurePlate)
        {
            out << " id " << t.id
                << " links " << (int)t.links.size();
            for (int id : t.links) out << ' ' << id;
        }

        out << "\n";
    }

    out << "enemies " << (int)lvl.enemies.size() << "\n";
    for (const auto& e : lvl.enemies)
    {
        out << "enemy " << EnemyPresetToString(e.preset) << ' '
            << e.pos.x << ' ' << e.pos.y << "\n";
    }

    out.flush();
    return out.good();
}

bool LoadLevelFromFile(const char* filename, LevelData& outLvl)
{
    if (!filename) return false;

    std::ifstream in(filename);
    if (!in) return false;

    outLvl = LevelData{};

    std::string word;
    int version = 0;

    in >> word >> version;
    if (!in || word != "version") return false;
    if (version != 1) return false;

    in >> word >> outLvl.rows >> outLvl.cols;
    if (!in || word != "size") return false;
    if (outLvl.rows <= 0 || outLvl.cols <= 0) return false;

    in >> word >> outLvl.spawn.x >> outLvl.spawn.y;
    if (!in || word != "spawn") return false;

    in >> word;
    if (!in || word != "tiles") return false;

    outLvl.tiles.assign((size_t)outLvl.rows * (size_t)outLvl.cols, (int)MapTile::Type::NONE);
    for (int y = 0; y < outLvl.rows; ++y)
    {
        for (int x = 0; x < outLvl.cols; ++x)
        {
            int v = 0;
            in >> v;
            if (!in) return false;
            outLvl.tiles[(size_t)y * (size_t)outLvl.cols + (size_t)x] = v;
        }
    }

    // traps block
    in >> word;
    if (!in || word != "traps") return false;

    int trapCount = 0;
    in >> trapCount;
    if (!in || trapCount < 0) return false;

    outLvl.traps.clear();
    outLvl.traps.reserve((size_t)trapCount);

    std::string line;
    std::getline(in, line); // consume rest of line

    for (int i = 0; i < trapCount; ++i)
    {
        std::getline(in, line);
        if (!in) return false;

        TrimLeft(line);
        if (line.empty()) { --i; continue; }

        std::istringstream ss(line);

        std::string tag;
        ss >> tag;
        if (!ss || tag != "trap") return false;

        std::string typeTok;
        ss >> typeTok;
        if (!ss) return false;

        TrapDefSimple t;
        if (!ParseTrapType(typeTok, t.type)) return false;

        ss >> t.pos.x >> t.pos.y >> t.size.x >> t.size.y;
        if (!ss) return false;

        while (ss)
        {
            std::string key;
            ss >> key;
            if (!ss) break;

            const Trap::Type tt = static_cast<Trap::Type>(t.type);

            if (tt == Trap::Type::SpikePlate)
            {
                if (key == "up") ss >> t.upTime;
                else if (key == "down") ss >> t.downTime;
                else if (key == "dmg") ss >> t.damageOnHit;
                else if (key == "startDisabled")
                {
                    int v = 0; ss >> v;
                    t.startDisabled = (v != 0);
                }
                else
                {
                    // unknown key, ignore token
                    std::string junk;
                    ss >> junk;
                }
            }
            else if (tt == Trap::Type::LavaPool)
            {
                if (key == "dpt") ss >> t.damagePerTick;
                else if (key == "tick") ss >> t.tickInterval;
                else
                {
                    std::string junk;
                    ss >> junk;
                }
            }
            else if (tt == Trap::Type::PressurePlate)
            {
                if (key == "id") ss >> t.id;
                else if (key == "links")
                {
                    int n = 0; ss >> n;
                    if (n < 0) n = 0;
                    t.links.clear();
                    t.links.reserve((size_t)n);
                    for (int k = 0; k < n; ++k)
                    {
                        int id = -1; ss >> id;
                        t.links.push_back(id);
                    }
                }
                else
                {
                    std::string junk;
                    ss >> junk;
                }
            }
            else
            {
                // unknown trap type: ignore any remaining tokens
                break;
            }
        }

        outLvl.traps.push_back(t);
    }

    // enemies block (optional for backward compatibility)
    outLvl.enemies.clear();

    // try to read next section; if file ends here, it's valid.
    if (!(in >> word))
        return true;

    if (word != "enemies")
    {
        // unknown section: ignore, but keep file load successful
        return true;
    }

    int enemyCount = 0;
    in >> enemyCount;
    if (!in || enemyCount < 0) return false;

    outLvl.enemies.reserve((size_t)enemyCount);

    std::getline(in, line); // consume rest of line
    for (int i = 0; i < enemyCount; ++i)
    {
        std::getline(in, line);
        if (!in) return false;

        TrimLeft(line);
        if (line.empty()) { --i; continue; }

        std::istringstream ss(line);

        std::string tag;
        ss >> tag;
        if (!ss || tag != "enemy") return false;

        std::string presetTok;
        ss >> presetTok;
        if (!ss) return false;

        EnemyDefSimple e;
        if (!ParseEnemyPreset(presetTok, e.preset)) return false;

        ss >> e.pos.x >> e.pos.y;
        if (!ss) return false;

        outLvl.enemies.push_back(e);
    }

    return true;
}

void BuildLevelDataFromEditor(
    MapGrid& grid, int rows, int cols,
    const std::vector<TrapDefSimple>& traps,
    const std::vector<EnemyDefSimple>& enemies,
    const AEVec2& spawn,
    LevelData& out)
{
    out = LevelData{};
    out.rows = rows;
    out.cols = cols;
    out.spawn = spawn;
    out.traps = traps;
    out.enemies = enemies;

    out.tiles.assign((size_t)rows * (size_t)cols, (int)MapTile::Type::NONE);

    for (int y = 0; y < rows; ++y)
    {
        for (int x = 0; x < cols; ++x)
        {
            const MapTile* t = grid.GetTile(x, y);
            out.tiles[(size_t)y * (size_t)cols + (size_t)x] =
                t ? (int)t->type : (int)MapTile::Type::NONE;
        }
    }
}

bool ApplyLevelDataToEditor(
    const LevelData& lvl,
    MapGrid*& ioGrid,
    std::vector<TrapDefSimple>& ioTraps,
    std::vector<EnemyDefSimple>& ioEnemies,
    AEVec2& ioSpawn)
{
    if (lvl.rows <= 0 || lvl.cols <= 0) return false;
    if ((int)lvl.tiles.size() != lvl.rows * lvl.cols) return false;

    delete ioGrid;
    ioGrid = new MapGrid(lvl.rows, lvl.cols);
    if (!ioGrid) return false;

    for (int y = 0; y < lvl.rows; ++y)
    {
        for (int x = 0; x < lvl.cols; ++x)
        {
            int v = lvl.tiles[(size_t)y * (size_t)lvl.cols + (size_t)x];
            if (v < 0) v = (int)MapTile::Type::NONE;
            if (v >= MapTile::typeCount) v = (int)MapTile::Type::NONE;

            ioGrid->SetTile(x, y, (MapTile::Type)v);
        }
    }

    ioTraps = lvl.traps;
    ioEnemies = lvl.enemies;
    ioSpawn = lvl.spawn;
    return true;
}