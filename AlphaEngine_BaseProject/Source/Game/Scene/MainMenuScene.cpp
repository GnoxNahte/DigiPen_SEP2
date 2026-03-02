#include "MainMenuScene.h"
#include "AEEngine.h"
#include "LevelIO.h"
#include "GSM.h"
#include "../Environment/MapTile.h"
#include "../Time.h"

#include <Windows.h>
#include <new>
#include <string>
#include <iostream>

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
{
}

MainMenuScene::~MainMenuScene() {}

void MainMenuScene::Init()
{
    // Set working directory and camera scale FIRST — before ANY asset loads
    SetCurrentDirectoryA(ExeDir().c_str());
    Camera::scale = 64.f;
    Camera::position = { 3.f, 3.f };
    AEGfxSetCamPosition(Camera::position.x * Camera::scale,
        Camera::position.y * Camera::scale);

    std::string path = "Assets\\Levels\\menu.lvl";
    LevelData lvl;

    if (LoadLevelFromFile(path.c_str(), lvl))
    {
        mapCols = lvl.cols;

        // reconstruct map so it reloads texture with correct working dir
        map.~MapGrid();
        new (&map) MapGrid(lvl.rows, lvl.cols);

        for (int y = 0; y < lvl.rows; ++y)
            for (int x = 0; x < lvl.cols; ++x)
            {
                int v = lvl.tiles[(size_t)y * lvl.cols + x];
                if (v < 0 || v >= MapTile::typeCount) v = 0;
                map.SetTile(x, y, (MapTile::Type)v);
            }

        player.Reset(lvl.spawn);
        std::cout << "[Menu] Loaded menu.lvl, spawn=" << lvl.spawn.x << "," << lvl.spawn.y << "\n";
    }
    else
    {
        mapCols = 40;
        // reconstruct map cleanly even in fallback
        map.~MapGrid();
        new (&map) MapGrid(20, 40);

        for (int x = 0; x < 40; ++x)
            map.SetTile(x, 1, MapTile::Type::GROUND);

        player.Reset({ 3.f, 3.f });
        std::cout << "[Menu] menu.lvl not found, using fallback\n";
    }

    // snap camera to spawn
    Camera::position = player.GetPosition();
    AEGfxSetCamPosition(Camera::position.x * Camera::scale,
        Camera::position.y * Camera::scale);

    std::cout << "[Menu] Camera::scale=" << Camera::scale
        << " pos=" << Camera::position.x << "," << Camera::position.y << "\n";
}

void MainMenuScene::Update()
{
    player.Update();

    float dt = static_cast<float>(Time::GetInstance().GetScaledDeltaTime());
    trapMgr.Update(dt, player);
    enemyMgr.UpdateAll(player.GetPosition(), map);

    Camera::position = player.GetPosition();
    AEGfxSetCamPosition(Camera::position.x * Camera::scale,
        Camera::position.y * Camera::scale);

    if (player.GetPosition().x > (float)mapCols)
    {
        gPendingLevelPath = ExeDir() + "Assets\\Levels\\level01.lvl";
        GSM::ChangeScene(SceneState::GS_GAME);
    }
}

void MainMenuScene::Render()
{
    AEGfxSetBackgroundColor(0.15f, 0.15f, 0.15f);

    // re-apply camera and scale every frame before drawing
    Camera::scale = 64.f;
    AEGfxSetCamPosition(Camera::position.x * Camera::scale,
        Camera::position.y * Camera::scale);

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.f);
    AEGfxSetColorToMultiply(1.f, 1.f, 1.f, 1.f);
    AEGfxSetColorToAdd(0.f, 0.f, 0.f, 0.f);

    map.Render();
    player.Render();
    enemyMgr.RenderAll();
}

void MainMenuScene::Exit() {}