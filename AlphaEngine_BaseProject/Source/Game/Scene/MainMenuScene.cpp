#include "MainMenuScene.h"
#include "AEEngine.h"
#include "LevelIO.h"
#include "GSM.h"
#include "../../Game/Rooms/RoomBuilder.h"
#include "../Environment/MapTile.h"
#include "../enemy/Enemy.h"
#include "../Time.h"
#include "../AudioManager.h"

#include <Windows.h>
#include <new>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>



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
    , enemyBoss()
    , camera({ 1.f, 1.f }, { 50.f, 50.f }, 64.f)
    , roomSystem(map, player, camera, trapMgr, enemyMgr, enemyBoss, roomMgr)
{
}

MainMenuScene::~MainMenuScene()
{
}

void MainMenuScene::LoadRunRecords()
{
    personalBest = RunRecord{};
    latestRun = RunRecord{};

    std::ifstream in("Assets/Levels/leaderboard.txt");
    if (!in.is_open())
        in.open("../../Assets/Levels/leaderboard.txt");

    if (!in.is_open())
        return;

    std::string label;
    int levels = 0;
    double time = 0.0;

    while (in >> label >> levels >> time)
    {
        if (label == "BEST")
        {
            personalBest.levelsCleared = levels;
            personalBest.timeSeconds = time;
            personalBest.valid = true;
        }
        else if (label == "LATEST")
        {
            latestRun.levelsCleared = levels;
            latestRun.timeSeconds = time;
            latestRun.valid = true;
        }
    }
}

std::string MainMenuScene::FormatTime(double seconds) const
{
    if (seconds < 0.0)
        seconds = 0.0;

    int totalMs = static_cast<int>(seconds * 1000.0);
    int mm = (totalMs / 60000) % 100;
    int ss = (totalMs / 1000) % 60;
    int cs = (totalMs / 10) % 100;

    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << mm
        << ":"
        << std::setfill('0') << std::setw(2) << ss
        << ":"
        << std::setfill('0') << std::setw(2) << cs;
    return oss.str();
}

void MainMenuScene::RenderLeaderboard() const
{
    if (uiFont < 0)
        return;

    
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.f);
    AEGfxSetColorToMultiply(1.f, 1.f, 1.f, 1.f);
    AEGfxSetColorToAdd(0.f, 0.f, 0.f, 0.f);

    AEGfxPrint((s8)uiFont, "RUN RECORDS", 0.42f, 0.72f, 0.95f, 1.f, 0.9f, 0.45f, 1.f);

    if (personalBest.valid)
    {
        std::string best1 = "BEST LEVELS: " + std::to_string(personalBest.levelsCleared);
        std::string best2 = "BEST TIME:   " + FormatTime(personalBest.timeSeconds);

        AEGfxPrint((s8)uiFont, best1.c_str(), 0.38f, 0.58f, 0.72f, 1.f, 1.f, 1.f, 1.f);
        AEGfxPrint((s8)uiFont, best2.c_str(), 0.38f, 0.49f, 0.72f, 1.f, 1.f, 1.f, 1.f);
    }
    else
    {
        AEGfxPrint((s8)uiFont, "BEST LEVELS: --", 0.38f, 0.58f, 0.72f, 1.f, 1.f, 1.f, 1.f);
        AEGfxPrint((s8)uiFont, "BEST TIME:   --:--:--", 0.38f, 0.49f, 0.72f, 1.f, 1.f, 1.f, 1.f);
    }

    if (latestRun.valid)
    {
        std::string latest1 = "LAST LEVELS: " + std::to_string(latestRun.levelsCleared);
        std::string latest2 = "LAST TIME:   " + FormatTime(latestRun.timeSeconds);

        AEGfxPrint((s8)uiFont, latest1.c_str(), 0.38f, 0.34f, 0.72f, 0.9f, 0.9f, 0.9f, 1.f);
        AEGfxPrint((s8)uiFont, latest2.c_str(), 0.38f, 0.25f, 0.72f, 0.9f, 0.9f, 0.9f, 1.f);
    }
    else
    {
        AEGfxPrint((s8)uiFont, "LAST LEVELS: --", 0.38f, 0.34f, 0.72f, 0.9f, 0.9f, 0.9f, 1.f);
        AEGfxPrint((s8)uiFont, "LAST TIME:   --:--:--", 0.38f, 0.25f, 0.72f, 0.9f, 0.9f, 0.9f, 1.f);
    }
}

void MainMenuScene::Init()
{
    SetCurrentDirectoryA(ExeDir().c_str());

    if (uiFont < 0)
    {
        uiFont = AEGfxCreateFont("Assets/buggy-font.ttf", 18);
        if (uiFont < 0) uiFont = AEGfxCreateFont("../Assets/buggy-font.ttf", 18);
        if (uiFont < 0) uiFont = AEGfxCreateFont("../../Assets/buggy-font.ttf", 18);
    }

    if (!fadeMesh)
    {
        AEGfxMeshStart();
        AEGfxTriAdd(-0.5f, -0.5f, 0xFF000000, 0.f, 0.f,
            0.5f, -0.5f, 0xFF000000, 0.f, 0.f,
            0.5f, 0.5f, 0xFF000000, 0.f, 0.f);
        AEGfxTriAdd(-0.5f, -0.5f, 0xFF000000, 0.f, 0.f,
            0.5f, 0.5f, 0xFF000000, 0.f, 0.f,
            -0.5f, 0.5f, 0xFF000000, 0.f, 0.f);
        fadeMesh = AEGfxMeshEnd();
    }

    isFadingToGame = false;
    fadeAlpha = 0.0f;
    LoadRunRecords();

    vineTexture = AEGfxTextureLoad("Assets/Tmp/vines.png");
    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.f, 1.f,
        0.5f, -0.5f, 0xFFFFFFFF, 1.f, 1.f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.f, 0.f);
    AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.f, 1.f,
        0.5f, 0.5f, 0xFFFFFFFF, 1.f, 0.f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.f, 0.f);
    vineMesh = AEGfxMeshEnd();

    SpikePlate::LoadSharedRenderResources();

    const std::string path = "..\\..\\Assets\\Levels\\mainmenufr.lvl";

    LevelData lvl;
    RoomID startRoom = ROOM_1;

    roomMgr.Clear();
    roomSystem.ClearBlockedReturnDir();
    vinePositions.clear();

    if (LoadLevelFromFile(path.c_str(), lvl))
    {
        mapCols = lvl.cols;
        mapRows = lvl.rows;

        map.~MapGrid();
        new (&map) MapGrid(lvl.cols, lvl.rows);

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

        vinePositions = lvl.vines;

        BuildRoomsFromLevelData(lvl, roomMgr, startRoom);
        roomMgr.SetCurrentRoom(startRoom);
    }
    else
    {
        mapCols = ROOM_COLS;
        mapRows = ROOM_ROWS;

        map.~MapGrid();
        new (&map) MapGrid(mapCols, mapRows);

        RoomData room{};
        room.id = ROOM_1;
        room.gridX = 0;
        room.gridY = 0;
        room.startSpawn = { 2.5f, 2.5f };
        room.rightRoom = ROOM_NONE;

        for (int y = 0; y < ROOM_ROWS; ++y)
        {
            for (int x = 0; x < ROOM_COLS; ++x)
            {
                room.tiles[y][x] = MapTile::Type::NONE;
            }
        }

        for (int x = 0; x < ROOM_COLS; ++x)
        {
            room.tiles[0][x] = MapTile::Type::GROUND_BODY;
            room.tiles[1][x] = MapTile::Type::GROUND_SURFACE;

            map.SetTile(x, 0, MapTile::Type::GROUND_BODY);
            map.SetTile(x, 1, MapTile::Type::GROUND_SURFACE);
        }

        roomMgr.SetRoom(ROOM_1, room);
        roomMgr.SetCurrentRoom(ROOM_1);
    }

    camera.~Camera();
    new (&camera) Camera(
        { 0.f, 0.f },
        { static_cast<float>(mapCols), static_cast<float>(mapRows) },
        64.f
    );

    roomSystem.BuildCurrentRoom();
    camera.Update();
    AudioManager::Init();
    AudioManager::PlayMenuMusic();
}

void MainMenuScene::Update()
{
    const float dt = static_cast<float>(Time::GetInstance().GetScaledDeltaTime());

    if (isFadingToGame)
    {
        fadeAlpha += fadeSpeed * dt;
        if (fadeAlpha > 1.0f)
            fadeAlpha = 1.0f;

        if (fadeAlpha >= 1.0f)
        {
            GSM::ChangeScene(SceneState::GS_GAME);
            return;
        }

        return;
    }

    player.Update();

    trapMgr.Update(dt, player);
    enemyMgr.UpdateAll(player.GetPosition(), map);
    camera.Update();
    AudioManager::Update();
    // std::cout << static_cast<int>(roomMgr.GetCurrentRoomID()); // TODO: Reference credits room id and add credits room music.
}

void MainMenuScene::Render()
{
    AEGfxSetBackgroundColor(0.15f, 0.15f, 0.15f);

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.f);
    AEGfxSetColorToMultiply(1.f, 1.f, 1.f, 1.f);
    AEGfxSetColorToAdd(0.f, 0.f, 0.f, 0.f);

    AEGfxSetCamPosition(Camera::position.x * Camera::scale,
        Camera::position.y * Camera::scale);

    map.Render();

    if (vineTexture && vineMesh)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1.f, 1.f, 1.f, 1.f);
        AEGfxSetColorToAdd(0.f, 0.f, 0.f, 0.f);
        AEGfxSetTransparency(1.f);
        AEGfxTextureSet(vineTexture, 0.f, 0.f);

        for (const auto& v : vinePositions)
        {
            AEMtx33 m;
            AEMtx33Trans(&m, v.x + 0.5f, v.y + 0.5f);
            AEMtx33ScaleApply(&m, &m, Camera::scale, Camera::scale);
            AEGfxSetTransform(m.m);
            AEGfxMeshDraw(vineMesh, AE_GFX_MDM_TRIANGLES);
        }
    }

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

        WorldToNDC(7.f, 11.f, nx, ny);
        AEGfxPrint((s8)uiFont, "AETHERFALL", nx, ny, 2.2f, 1.f, 1.f, 1.f, 1.f);

        WorldToNDC(21.f, 5.f, nx, ny);
        AEGfxPrint((s8)uiFont, "START GAME", nx, ny, 0.9f, 1.f, 0.82f, 0.35f, 1.f);
    }

    RenderLeaderboard();

    if (fadeAlpha > 0.0f && fadeMesh)
    {
        AEGfxSetCamPosition(0.f, 0.f);
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(fadeAlpha);
        AEGfxSetColorToMultiply(1.f, 1.f, 1.f, 1.f);
        AEGfxSetColorToAdd(0.f, 0.f, 0.f, 0.f);

        AEMtx33 scale, transform;
        AEMtx33Scale(&scale, (float)AEGfxGetWindowWidth(), (float)AEGfxGetWindowHeight());
        transform = scale;

        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(fadeMesh, AE_GFX_MDM_TRIANGLES);
    }
}

void MainMenuScene::Exit()
{
    if (vineTexture) { AEGfxTextureUnload(vineTexture); vineTexture = nullptr; }
    if (vineMesh) { AEGfxMeshFree(vineMesh); vineMesh = nullptr; }
    vinePositions.clear();

    SpikePlate::UnloadSharedRenderResources();

    if (fadeMesh) { AEGfxMeshFree(fadeMesh); fadeMesh = nullptr; }

    if (uiFont >= 0)
    {
        AEGfxDestroyFont((s8)uiFont);
        uiFont = -1;
    }

    roomSystem.ClearRuntimeRoomObjects();
    roomMgr.Clear();
    AudioManager::Exit();
}