#include "MainMenuScene.h"
#include "AEEngine.h"
#include "LevelIO.h"
#include "GSM.h"
#include "GameScene.h"
#include "../Environment/MapTile.h"
#include "../enemy/Enemy.h"
#include "../Time.h"
#include "../UI.h"
#include "../AudioManager.h"

#include <Windows.h>
#include <new>
#include <string>

// defined in GameScene.cpp
extern std::string gPendingLevelPath;

namespace
{
    static RoomDirection CheckMenuExit(const AEVec2& playerPos, int mapRows)
    {
        const bool nearLeft = playerPos.x <= 2.0f;
        const bool nearTop = playerPos.y >= (float)mapRows - 2.0f;

        if (nearLeft && nearTop)
            return DIR_LEFT;

        return DIR_NONE;
    }
}

AEGfxVertexList* MainMenuScene::MakeSpikeMesh(int frame)
{
    const float uMin = frame * 0.25f;
    const float uMax = (frame + 1) * 0.25f;
    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, uMin, 1.f,
        0.5f, -0.5f, 0xFFFFFFFF, uMax, 1.f,
        -0.5f, 0.5f, 0xFFFFFFFF, uMin, 0.f);
    AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, uMax, 1.f,
        0.5f, 0.5f, 0xFFFFFFFF, uMax, 0.f,
        -0.5f, 0.5f, 0xFFFFFFFF, uMin, 0.f);
    return AEGfxMeshEnd();
}

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
    AudioManager::LoadAll();

    if (uiFont < 0)
    {
        uiFont = AEGfxCreateFont("Assets/buggy-font.ttf", 18);
        if (uiFont < 0) uiFont = AEGfxCreateFont("../Assets/buggy-font.ttf", 18);
        if (uiFont < 0) uiFont = AEGfxCreateFont("../../Assets/buggy-font.ttf", 18);
    }

    // load vine texture and mesh
    vineTexture = AEGfxTextureLoad("Assets/Tmp/vines.png");
    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.f, 1.f,
        0.5f, -0.5f, 0xFFFFFFFF, 1.f, 1.f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.f, 0.f);
    AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.f, 1.f,
        0.5f, 0.5f, 0xFFFFFFFF, 1.f, 0.f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.f, 0.f);
    vineMesh = AEGfxMeshEnd();

    // load spike texture and per-frame meshes
    spikeTexture = AEGfxTextureLoad("Assets/Tmp/spikes.png");
    for (int i = 0; i < 4; ++i)
        spikeMeshes[i] = MakeSpikeMesh(i);

    std::string path = "..\\..\\Assets\\Levels\\gamewholelv.lvl";

    LevelData lvl;
    if (LoadLevelFromFile(path.c_str(), lvl))
    {
        mapCols = lvl.cols;
        mapRows = lvl.rows;

        // reconstruct map safely (no shallow copy)
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

        player.Reset(lvl.spawn);

        vinePositions = lvl.vines;

        // spawn traps and wire up pressure plate links
        std::vector<SpikePlate*> spawnedSpikes;
        std::vector<PressurePlate*> spawnedPlates;

        for (const auto& td : lvl.traps)
        {
            Box box{};
            box.size = td.size;
            box.position = AEVec2{ td.pos.x - td.size.x * 0.5f, td.pos.y - td.size.y * 0.5f };

            const Trap::Type tt = static_cast<Trap::Type>(td.type);

            if (tt == Trap::Type::SpikePlate)
            {
                SpikePlate& s = trapMgr.Spawn<SpikePlate>(
                    box, td.upTime, td.downTime, td.damageOnHit, td.startDisabled);
                spawnedSpikes.push_back(&s);
                spikeTraps.push_back(&s);
                spikeAnims.emplace_back();
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
            spawns.push_back({ (EnemySpawnType)ed.preset, ed.pos });

        if (!spawns.empty())
        {
            enemyMgr.SetSpawns(spawns);
            enemyMgr.SpawnAll();
        }
    }
    else
    {
        mapCols = 40;
        mapRows = 50;

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

    UI::Init(&player);
    AudioManager::PlayMusic(MusicId::MainMenu);
}

void MainMenuScene::Update()
{
    player.Update();

    float dt = static_cast<float>(Time::GetInstance().GetScaledDeltaTime());
    trapMgr.Update(dt, player);

    // tick spike animators
    for (int i = 0; i < (int)spikeTraps.size(); ++i)
    {
        SpikeAnim& a = spikeAnims[i];

        if (!a.triggered && spikeTraps[i]->IsEnabled())
            a.triggered = true;

        if (!a.triggered || a.done) continue;

        a.timer += dt;
        if (a.timer >= 0.08f)
        {
            a.timer -= 0.08f;
            if (++a.frame >= 3) { a.frame = 3; a.done = true; }
        }
    }

    enemyMgr.UpdateAll(player.GetPosition(), map);

    camera.Update();

    // transition to game when player reaches the exit area
    RoomDirection exitDir = CheckMenuExit(player.GetPosition(), mapRows);
    if (exitDir == DIR_LEFT)
    {
        gPendingLevelPath = ExeDir() + "..\\..\\Assets\\Levels\\checktransit.lvl";
        std::cout << "Setting pending path to: " << gPendingLevelPath << "\n";
        GSM::ChangeScene(SceneState::GS_GAME);
        return;
    }

    UI::GetDamageTextSpawner().Update();
    UI::Update();
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

    // vine decorations
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

    // spike animated sprites
    if (spikeTexture)
    {
        int idx = 0;
        for (const auto& a : spikeAnims)
        {
            Trap* t = spikeTraps[idx++];

            // 1. getting the center of the trap's box, so we can lock the spike sprite to it. This is important because some traps (like the lava pool) are larger than 1 tile, and we want the spike animation to be centered on the trap, not skewed to a corner.
            float centerX = t->GetBox().position.x + t->GetBox().size.x * 0.5f;
            float centerY = t->GetBox().position.y + t->GetBox().size.y * 0.5f;

            AEMtx33 m;
            // 2. pass in the center coordinates to the translation function, so the spike sprite will be drawn centered on the trap's box.
            AEMtx33Trans(&m, centerX, centerY);
            AEMtx33ScaleApply(&m, &m, Camera::scale, Camera::scale);

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1.f, 1.f, 1.f, 1.f);
            AEGfxSetColorToAdd(0.f, 0.f, 0.f, 0.f);
            AEGfxSetTransparency(1.f);
            AEGfxTextureSet(spikeTexture, 0.f, 0.f);
            AEGfxSetTransform(m.m);
            AEGfxMeshDraw(spikeMeshes[a.frame], AE_GFX_MDM_TRIANGLES);
        }
    }
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

    UI::Render();
}

void MainMenuScene::Exit()
{
    if (vineTexture) { AEGfxTextureUnload(vineTexture); vineTexture = nullptr; }
    if (vineMesh) { AEGfxMeshFree(vineMesh);         vineMesh = nullptr; }
    vinePositions.clear();

    if (spikeTexture) { AEGfxTextureUnload(spikeTexture); spikeTexture = nullptr; }
    for (int i = 0; i < 4; ++i)
        if (spikeMeshes[i]) { AEGfxMeshFree(spikeMeshes[i]); spikeMeshes[i] = nullptr; }
    spikeTraps.clear();
    spikeAnims.clear();

    if (uiFont >= 0)
    {
        AEGfxDestroyFont((s8)uiFont);
        uiFont = -1;
    }

    UI::Exit();
}