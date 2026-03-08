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
        std::vector<SpikePlate*> spawnedSpikes;
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

        // every pressure plate triggers every spike plate
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

        // simple fallback terrain using new tile enums
        for (int x = 0; x < 40; ++x)
        {
            map.SetTile(x, 0, MapTile::Type::GROUND_BODY);
            map.SetTile(x, 1, MapTile::Type::GROUND_SURFACE);
        }

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

    // transition to game when player reaches the exit area
    if (player.GetPosition().x >= 3.f && player.GetPosition().x <= 4.f &&
        player.GetPosition().y >= 48.f)
    {
        gPendingLevelPath = ExeDir() + "Assets\\Levels\\level01.lvl";
        GSM::ChangeScene(SceneState::GS_GAME);
    }
}

void MainMenuScene::Render()
{
    AEGfxSetBackgroundColor(0.15f, 0.15f, 0.15f);

    // reset render state
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

    if (uiFont >= 0)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.f);
        AEGfxSetColorToMultiply(1.f, 1.f, 1.f, 1.f);
        AEGfxSetColorToAdd(0.f, 0.f, 0.f, 0.f);

        auto WorldToNDC = [](float wx, float wy, float& ndcX, float& ndcY)
            {
                float screenX = (wx - Camera::position.x) * Camera::scale + AEGfxGetWindowWidth() * 0.5f;
                float screenY = (wy - Camera::position.y) * Camera::scale + AEGfxGetWindowHeight() * 0.5f;
                ndcX = (screenX / AEGfxGetWindowWidth()) * 2.f - 1.f;
                ndcY = (screenY / AEGfxGetWindowHeight()) * 2.f - 1.f;
            };

        float nx, ny;

        WorldToNDC(7.f, 12.f, nx, ny);
        AEGfxPrint((s8)uiFont, "AETHERFALL", nx, ny, 2.2f, 1.0f, 1.0f, 1.0f, 1.0f);

        WorldToNDC(7.f, 11.f, nx, ny);
        AEGfxPrint((s8)uiFont, "CONTROLS", nx, ny, 1.0f, 1.0f, 0.82f, 0.35f, 1.0f);

        WorldToNDC(7.f, 10.f, nx, ny);
        AEGfxPrint((s8)uiFont, "A / D  - Move", nx, ny, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);

        WorldToNDC(7.f, 9.f, nx, ny);
        AEGfxPrint((s8)uiFont, "SPACE - Jump", nx, ny, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);

        WorldToNDC(30.f, 26.f, nx, ny);
        AEGfxPrint((s8)uiFont, "Press Z to dash", nx, ny, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f);

        WorldToNDC(21.f, 10.f, nx, ny);
        AEGfxPrint((s8)uiFont, "Pressure plates activates the spikes", nx, ny, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f);

        WorldToNDC(35.f, 8.f, nx, ny);
        AEGfxPrint((s8)uiFont, "Press Z to attack", nx, ny, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f);

        WorldToNDC(9.f, 30.f, nx, ny);
        AEGfxPrint((s8)uiFont, "Good Job!", nx, ny, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f);
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