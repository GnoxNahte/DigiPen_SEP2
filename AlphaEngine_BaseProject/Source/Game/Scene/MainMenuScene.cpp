#include "MainMenuScene.h"
#include "AEEngine.h"
#include "LevelIO.h"
#include "GSM.h"
#include "GameScene.h"
#include "../Environment/MapTile.h"
#include "../enemy/Enemy.h"
#include "../Time.h"

#include <Windows.h>
#include <new>
#include <string>

// defined in GameScene.cpp
extern std::string gPendingLevelPath;

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
    , camera({ 1, 1 }, { 50, 50 }, 64)
{
}

MainMenuScene::~MainMenuScene()
{
}

void MainMenuScene::Init()
{
    // ensure relative texture paths work
    SetCurrentDirectoryA(ExeDir().c_str());

    if (uiFont < 0)
    {
        uiFont = AEGfxCreateFont("Assets/buggy-font.ttf", 18);
        if (uiFont < 0) uiFont = AEGfxCreateFont("../Assets/buggy-font.ttf", 18);
        if (uiFont < 0) uiFont = AEGfxCreateFont("../../Assets/buggy-font.ttf", 18);
    }

    std::string path = "Assets\\Levels\\menu.lvl";

    LevelData lvl;
    if (LoadLevelFromFile(path.c_str(), lvl))
    {
        mapCols = lvl.cols;

        // reconstruct map safely (no shallow copy)
        map.~MapGrid();
        new (&map) MapGrid(lvl.rows, lvl.cols);

        for (int y = 0; y < lvl.rows; ++y)
        {
            for (int x = 0; x < lvl.cols; ++x)
            {
                int v = lvl.tiles[(size_t)y * lvl.cols + x];
                if (v < 0 || v >= MapTile::typeCount)
                    v = 0;

                map.SetTile(x, y, (MapTile::Type)v);
            }
        }

        player.Reset(lvl.spawn);

        // spawn traps and wire up pressure plate links
        std::vector<SpikePlate*>    spawnedSpikes;
        std::vector<PressurePlate*> spawnedPlates;

        for (const auto& td : lvl.traps)
        {
            Box box{ td.pos, td.size };
            const Trap::Type tt = static_cast<Trap::Type>(td.type);

            if (tt == Trap::Type::SpikePlate)
            {
                SpikePlate& s = trapMgr.Spawn<SpikePlate>(
                    box, td.upTime, td.downTime, td.damageOnHit, td.startDisabled);
                spawnedSpikes.push_back(&s);
            }
            else if (tt == Trap::Type::PressurePlate)
            {
                PressurePlate& p = trapMgr.Spawn<PressurePlate>(box);
                spawnedPlates.push_back(&p);
            }
            else if (tt == Trap::Type::LavaPool)
            {
                trapMgr.Spawn<LavaPool>(box, td.damagePerTick, td.tickInterval);
            }
        }

        // every pressure plate triggers every spike plate (same rule as editor)
        for (PressurePlate* plate : spawnedPlates)
            for (SpikePlate* spike : spawnedSpikes)
                plate->AddLinkedTrap(spike);

        // spawn enemies
        std::vector<EnemyManager::SpawnInfo> spawns;
        for (const auto& ed : lvl.enemies)
            spawns.push_back({ (Enemy::Preset)ed.preset, ed.pos });

        if (!spawns.empty())
        {
            enemyMgr.SetSpawns(spawns);
            enemyMgr.SpawnAll();
        }
    }
    else
    {
        mapCols = 40;

        for (int x = 0; x < 40; ++x)
            map.SetTile(x, 1, MapTile::Type::GROUND);

        player.Reset({ 3.f, 3.f });
    }

    camera.SetFollow(&player.GetPosition(), 0, 0, true);
    camera.smoothTime = 0.4f;
}

void MainMenuScene::Update()
{
    player.Update();

    float dt = static_cast<float>(Time::GetInstance().GetScaledDeltaTime());
    trapMgr.Update(dt, player);
    enemyMgr.UpdateAll(player.GetPosition(), map);

    
    camera.Update();

    // transition to game when walking past right edge
    if (player.GetPosition().x > (float)mapCols)
    {
        gPendingLevelPath = ExeDir() + "Assets\\Levels\\menu.lvl";
        GSM::ChangeScene(SceneState::GS_GAME);
    }
}

void MainMenuScene::Render()
{
    AEGfxSetBackgroundColor(0.15f, 0.15f, 0.15f);

    // reset render state — UI::DrawHealthVignette leaves transparency=0 at full health
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.f);
    AEGfxSetColorToMultiply(1.f, 1.f, 1.f, 1.f);
    AEGfxSetColorToAdd(0.f, 0.f, 0.f, 0.f);

    AEGfxSetCamPosition(Camera::position.x * Camera::scale,
        Camera::position.y * Camera::scale);

    map.Render();
    trapMgr.Render();
    player.Render();
    enemyMgr.RenderAll();

    // always-visible menu text overlay
    if (uiFont >= 0)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.f);
        AEGfxSetColorToMultiply(1.f, 1.f, 1.f, 1.f);
        AEGfxSetColorToAdd(0.f, 0.f, 0.f, 0.f);

        // title
        AEGfxPrint((s8)uiFont, "GAME TITLE", -0.93f, 0.90f, 2.2f, 1.0f, 1.0f, 1.0f, 1.0f);

        // controls block
        AEGfxPrint((s8)uiFont, "CONTROLS", -0.93f, 0.74f, 1.0f, 1.0f, 0.82f, 0.35f, 1.0f);
        AEGfxPrint((s8)uiFont, "A / D  - Move", -0.93f, 0.64f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxPrint((s8)uiFont, "SPACE - Jump", -0.93f, 0.56f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxPrint((s8)uiFont, "Run right to begin", -0.93f, 0.46f, 1.0f, 1.0f, 0.95f, 0.85f, 0.25f);
    }
}

void MainMenuScene::Exit()
{
    if (uiFont >= 0)
    {
        AEGfxDestroyFont((s8)uiFont);
        uiFont = -1;
    }
}