#include "MainMenuScene.h"
#include "AEEngine.h"
#include "LevelIO.h"
#include "../Environment/MapTile.h"
#include "../Time.h"

#include <string>
#include <new> // placement new

// defined here, extern'd in GameScene.cpp
std::string gPendingLevelPath;

std::string MainMenuScene::ExeDir()
{
    char buf[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    std::string s(buf);
    size_t slash = s.find_last_of("\\/");
    return (slash != std::string::npos) ? s.substr(0, slash + 1) : ".\\";
}

MainMenuScene::MainMenuScene()
    : map(20, 40)
    , player(&map, &enemyMgr)
    , camera({ 1,1 }, { 39, 19 }, 64)
{
    camera.SetFollow(&player.GetPosition(), 0, 40, true);
}

MainMenuScene::~MainMenuScene() {}

void MainMenuScene::Init()
{
    std::string path = ExeDir() + "Assets\\Levels\\menu.lvl";

    LevelData lvl;
    if (LoadLevelFromFile(path.c_str(), lvl))
    {
        mapCols = lvl.cols;

        // IMPORTANT:
        // MapGrid owns engine resources (mesh/texture) via raw pointers.
        // Doing `map = MapGrid(...)` performs a shallow copy assignment, then the temporary
        // frees those resources in its destructor -> `map` holds dangling pointers -> crash.
        // Reconstruct the object in-place so the address (&map) stays stable for Player.
        map.~MapGrid();
        new (&map) MapGrid(lvl.rows, lvl.cols);

        camera = Camera({ 1,1 }, { (float)lvl.cols - 1, (float)lvl.rows - 1 }, 64);
        camera.SetFollow(&player.GetPosition(), 0, (float)lvl.cols, true);

        for (int y = 0; y < lvl.rows; ++y)
            for (int x = 0; x < lvl.cols; ++x)
            {
                int v = lvl.tiles[(size_t)y * lvl.cols + x];
                if (v < 0 || v >= MapTile::typeCount) v = 0;
                map.SetTile(x, y, (MapTile::Type)v);
            }

        for (const auto& td : lvl.traps)
        {
            Box box{ td.pos, td.size };
            const Trap::Type tt = static_cast<Trap::Type>(td.type);
            if (tt == Trap::Type::SpikePlate)
                trapMgr.Spawn<SpikePlate>(box, td.upTime, td.downTime, td.damageOnHit, td.startDisabled);
            else if (tt == Trap::Type::PressurePlate)
                trapMgr.Spawn<PressurePlate>(box);
            else if (tt == Trap::Type::LavaPool)
                trapMgr.Spawn<LavaPool>(box, td.damagePerTick, td.tickInterval);
        }

        std::vector<EnemyManager::SpawnInfo> spawns;
        for (const auto& ed : lvl.enemies)
            spawns.push_back({ (Enemy::Preset)ed.preset, ed.pos });
        enemyMgr.SetSpawns(spawns);
        enemyMgr.SpawnAll();

        player.Reset(lvl.spawn);
    }
    else
    {
        // menu.lvl not built yet — flat ground fallback
        mapCols = 40;
        for (int x = 0; x < 40; ++x)
            map.SetTile(x, 1, MapTile::Type::GROUND);
        player.Reset({ 3.f, 3.f });
    }
}

void MainMenuScene::Update()
{
    player.Update();
    camera.Update();

    float dt = static_cast<float>(Time::GetInstance().GetScaledDeltaTime());
    trapMgr.Update(dt, player);
    enemyMgr.UpdateAll(player.GetPosition(), map);

    // player walks off the right edge → go to level01
    if (player.GetPosition().x > (float)mapCols)
    {
        gPendingLevelPath = ExeDir() + "Assets\\Levels\\level01.lvl";
        GSM::ChangeScene(SceneState::GS_GAME);
    }
}

void MainMenuScene::Render()
{
    map.Render();
    player.Render();
    enemyMgr.RenderAll();
}

void MainMenuScene::Exit()
{
    // nothing to clean up — members destroy themselves
}