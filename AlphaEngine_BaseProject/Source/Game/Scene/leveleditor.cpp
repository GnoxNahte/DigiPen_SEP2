// leveleditor.cpp
#include "AEEngine.h"
#include "leveleditor.h"

#include "../Environment/MapGrid.h"
#include "../Environment/MapTile.h"
#include "../Camera.h"
#include "../../Utils/AEExtras.h"

#include "../../EditorUI.h"
#include "LevelIO.h"
#include "../Environment/traps.h"
#include "../Player/Player.h"
#include "../enemy/Enemy.h"
#include "../enemy/EnemyManager.h"
#include "../enemy/AttackSystem.h"

#include "../Time.h"
#include "../UI.h"

#include <iostream>
#include <vector>
#include <cmath>
#include <cstdio>
#include <string>

/*========================================================
    configuration
========================================================*/

static constexpr int   GRID_ROWS = 50;
static constexpr int   GRID_COLS = 50;
static constexpr float CAMERA_SCALE = 64.0f;
static constexpr float CAMERA_SPEED = 10.0f;
static constexpr float ZOOM_SPEED = 0.05f;
static constexpr float ZOOM_MIN = 16.0f;
static constexpr float ZOOM_MAX = 128.0f;

/*========================================================
    editor state
========================================================*/

static MapGrid* gMap = nullptr;
static Camera* gCamera = nullptr;

static EditorUIState gUI{};
static EditorUIIO    gUIIO{};
static s8            gUIFont = -1;

static std::vector<TrapDefSimple>  gTrapDefs;
static std::vector<EnemyDefSimple> gEnemyDefs;
static AEVec2                      gSpawn{ 5.0f, 5.0f };

static float gSaveMessageTimer = 0.f;
static bool  gSaveSuccess = false;
static bool  gHoverCellValid = false;
static int   gHoverCellX = -1;
static int   gHoverCellY = -1;

// ── spike animation state ─────────────────────────────────────────────────
static AEGfxTexture* gSpikeTexture = nullptr;
static AEGfxVertexList* gSpikeMeshes[4] = {};

struct SpikeAnim
{
    bool  triggered = false;
    bool  done = false;
    int   frame = 0;
    float timer = 0.f;
};
static std::vector<SpikeAnim> gSpikeAnims;
static std::vector<Trap*>     gSpikeTraps;

// ── vine decoration state ─────────────────────────────────────────────────
static AEGfxTexture* gVineTexture = nullptr;
static AEGfxVertexList* gVineMesh = nullptr;
static std::vector<AEVec2> gVinePositions;  // grid cell coords (x, y)  // raw ptrs into gPlayTraps, parallel to gSpikeAnims

/*========================================================
    save prompt state
========================================================*/

static bool        gPromptActive = false;
static bool        gPromptIsSave = true;
static std::string gPromptInput = "";
static bool        gPromptJustOpened = false;

static std::string NormaliseFilename(const std::string& name)
{
    char exePath[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);

    std::string dir(exePath);
    size_t slash = dir.find_last_of("\\/");
    if (slash != std::string::npos)
        dir = dir.substr(0, slash + 1);
    else
        dir = ".\\";

    std::string folder = dir + "..\\..\\Assets\\Levels\\";

    CreateDirectoryA((dir + "Assets").c_str(), nullptr);
    CreateDirectoryA(folder.c_str(), nullptr);

    std::string filename = name;
    if (filename.size() < 4 || filename.substr(filename.size() - 4) != ".lvl")
        filename += ".lvl";

    return folder + filename;
}

/*========================================================
    play mode state
========================================================*/

static Player* gPlayPlayer = nullptr;
static TrapManager* gPlayTraps = nullptr;
static EnemyManager* gPlayEnemies = nullptr;
static Camera* gPlayCamera = nullptr;
static EnemyBoss* gPlayBoss = nullptr;

AttackSystem attackSystem;

/*========================================================
    helpers
========================================================*/

static bool InBounds(int x, int y)
{
    return x >= 0 && x < GRID_ROWS && y >= 0 && y < GRID_COLS;
}

static void ApplyWorldCamera()
{
    if (!gCamera) return;
    AEGfxSetCamPosition(
        gCamera->position.x * Camera::scale,
        gCamera->position.y * Camera::scale
    );
    Camera::position = gCamera->position;
}

// ── trap helpers ─────────────────────────────────────────────────────────────

static void RemoveTrapsAtCell(int tx, int ty)
{
    for (size_t i = 0; i < gTrapDefs.size();)
    {
        int ex = (int)std::floor(gTrapDefs[i].pos.x);
        int ey = (int)std::floor(gTrapDefs[i].pos.y);
        if (ex == tx && ey == ty)
            gTrapDefs.erase(gTrapDefs.begin() + (ptrdiff_t)i);
        else
            ++i;
    }
}

static void PlaceTrapAtCell(int tx, int ty, Trap::Type trapType)
{
    TrapDefSimple t{};
    t.type = (int)trapType;
    t.pos = AEVec2{ tx + 0.5f, ty + 0.5f };
    t.size = AEVec2{ 1.0f, 1.0f };
    t.upTime = 1.0f;
    t.downTime = 1.0f;
    t.damageOnHit = 10;
    if (trapType == Trap::Type::SpikePlate)
        t.startDisabled = true;
    else
        t.startDisabled = false;

    for (auto& e : gTrapDefs)
    {
        int ex = (int)std::floor(e.pos.x);
        int ey = (int)std::floor(e.pos.y);
        if (ex == tx && ey == ty && e.type == t.type) { e = t; return; }
    }
    gTrapDefs.push_back(t);
}

// ── enemy helpers ─────────────────────────────────────────────────────────────

static void RemoveEnemyAtCell(int tx, int ty)
{
    for (size_t i = 0; i < gEnemyDefs.size();)
    {
        int ex = (int)std::floor(gEnemyDefs[i].pos.x);
        int ey = (int)std::floor(gEnemyDefs[i].pos.y);
        if (ex == tx && ey == ty)
            gEnemyDefs.erase(gEnemyDefs.begin() + (ptrdiff_t)i);
        else
            ++i;
    }
}

static void PlaceEnemyAtCell(int tx, int ty, EnemySpawnType& type)
{
    RemoveEnemyAtCell(tx, ty);
    EnemyDefSimple e{};
    e.preset = (int)type;
    e.pos = AEVec2{ tx + 0.5f, ty + 0.f };
    gEnemyDefs.push_back(e);
}

// ── spike mesh builder ────────────────────────────────────────────────────────

// Builds a unit quad with UVs baked for one horizontal frame of the sprite sheet.
// Sheet is 64x16 => 4 frames of 16px => each frame = 0.25 UV width.
static AEGfxVertexList* MakeSpikeMesh(int frame)
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

// ── overlay quad ─────────────────────────────────────────────────────────────

static AEGfxVertexList* gOverlayQuad = nullptr;

static void OverlayInit()
{
    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0, 1, 0.5f, -0.5f, 0xFFFFFFFF, 1, 1, -0.5f, 0.5f, 0xFFFFFFFF, 0, 0);
    AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1, 1, 0.5f, 0.5f, 0xFFFFFFFF, 1, 0, -0.5f, 0.5f, 0xFFFFFFFF, 0, 0);
    gOverlayQuad = AEGfxMeshEnd();
}

static void OverlayShutdown()
{
    if (gOverlayQuad) { AEGfxMeshFree(gOverlayQuad); gOverlayQuad = nullptr; }
}

static void DrawWorldRect(float worldX, float worldY, float worldW, float worldH,
    float r, float g, float b, float a)
{
    float cx = (worldX + worldW * 0.5f) * Camera::scale;
    float cy = (worldY + worldH * 0.5f) * Camera::scale;

    AEMtx33 sc, ro, tr, m;
    AEMtx33Rot(&ro, 0.f);
    AEMtx33Scale(&sc, worldW * Camera::scale, worldH * Camera::scale);
    AEMtx33Trans(&tr, cx, cy);
    AEMtx33Concat(&m, &ro, &sc);
    AEMtx33Concat(&m, &tr, &m);

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransform(m.m);
    AEGfxSetColorToMultiply(r, g, b, a);
    AEGfxMeshDraw(gOverlayQuad, AE_GFX_MDM_TRIANGLES);
}

static void DrawScreenRect(float cx, float cy, float w, float h,
    float r, float g, float b, float a)
{
    AEGfxSetCamPosition(
        (float)AEGfxGetWindowWidth() * 0.5f,
        (float)AEGfxGetWindowHeight() * 0.5f
    );

    AEMtx33 sc, ro, tr, m;
    AEMtx33Rot(&ro, 0.f);
    AEMtx33Scale(&sc, w, h);
    AEMtx33Trans(&tr, cx, cy);
    AEMtx33Concat(&m, &ro, &sc);
    AEMtx33Concat(&m, &tr, &m);

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransform(m.m);
    AEGfxSetColorToMultiply(r, g, b, a);
    AEGfxMeshDraw(gOverlayQuad, AE_GFX_MDM_TRIANGLES);
}

static void DrawScreenText(const char* text, float px, float py,
    float r, float g, float b)
{
    int winW = AEGfxGetWindowWidth();
    int winH = AEGfxGetWindowHeight();
    float ndcX = (px / (float)winW) * 2.f - 1.f;
    float ndcY = (py / (float)winH) * 2.f - 1.f;

    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToAdd(0, 0, 0, 0);
    AEGfxSetColorToMultiply(1, 1, 1, 1);
    AEGfxSetTransparency(1.f);
    AEGfxPrint(gUIFont, text, ndcX, ndcY, 1.0f, r, g, b, 1.f);
}

static void DrawGridOverlay()
{
    if (!gMap) return;

    const float thickness = 0.03f;

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToAdd(0, 0, 0, 0);

    for (int x = 0; x <= GRID_COLS; ++x)
    {
        DrawWorldRect(
            (float)x - thickness * 0.5f, 0.0f,
            thickness, (float)GRID_ROWS,
            1.0f, 1.0f, 1.0f, 0.18f
        );
    }

    for (int y = 0; y <= GRID_ROWS; ++y)
    {
        DrawWorldRect(
            0.0f, (float)y - thickness * 0.5f,
            (float)GRID_COLS, thickness,
            1.0f, 1.0f, 1.0f, 0.18f
        );
    }
}

/*========================================================
    save / load prompt
========================================================*/

static void Prompt_Open(bool isSave)
{
    gPromptActive = true;
    gPromptIsSave = isSave;
    gPromptInput = "";
    gPromptJustOpened = true;
}

static void Prompt_Update()
{
    if (!gPromptActive) return;
    if (gPromptJustOpened) { gPromptJustOpened = false; return; }

    bool shift = AEInputCheckCurr(AEVK_LSHIFT) || AEInputCheckCurr(AEVK_RSHIFT);

    for (u8 vk = AEVK_A; vk <= AEVK_Z; ++vk)
    {
        if (AEInputCheckTriggered(vk) && gPromptInput.size() < 32)
        {
            char c = shift ? (char)vk : (char)(vk + 32);
            gPromptInput += c;
        }
    }

    for (u8 vk = AEVK_0; vk <= AEVK_9; ++vk)
        if (AEInputCheckTriggered(vk) && gPromptInput.size() < 32)
            gPromptInput += (char)vk;

    if (AEInputCheckTriggered(AEVK_MINUS) && gPromptInput.size() < 32)
        gPromptInput += shift ? '_' : '-';

    if (AEInputCheckTriggered(AEVK_BACK) && !gPromptInput.empty())
        gPromptInput.pop_back();

    if (AEInputCheckTriggered(AEVK_RETURN) && !gPromptInput.empty())
    {
        std::string path = NormaliseFilename(gPromptInput);

        if (gPromptIsSave)
        {
            LevelData lvl;
            BuildLevelDataFromEditor(*gMap, GRID_ROWS, GRID_COLS,
                gTrapDefs, gEnemyDefs, gVinePositions, gSpawn, lvl);
            gSaveSuccess = SaveLevelToFile(path.c_str(), lvl);
            gSaveMessageTimer = 2.5f;
        }
        else
        {
            LevelData lvl;
            if (LoadLevelFromFile(path.c_str(), lvl))
            {
                ApplyLevelDataToEditor(lvl, gMap, gTrapDefs, gEnemyDefs, gVinePositions, gSpawn);
                gSaveSuccess = true;
                gSaveMessageTimer = 2.5f;
            }
            else
            {
                gSaveSuccess = false;
                gSaveMessageTimer = 2.5f;
            }
        }

        gPromptActive = false;
    }

    if (AEInputCheckTriggered(AEVK_Z))
        gPromptActive = false;
}

static void Prompt_Draw()
{
    if (!gPromptActive) return;

    int winW = AEGfxGetWindowWidth();
    int winH = AEGfxGetWindowHeight();

    float cx = winW * 0.5f;
    float cy = winH * 0.5f;

    DrawScreenRect(cx, cy, (float)winW, (float)winH, 0.f, 0.f, 0.f, 0.55f);

    float bw = 420.f, bh = 160.f;
    DrawScreenRect(cx, cy, bw, bh, 0.15f, 0.15f, 0.18f, 0.97f);

    const char* title = gPromptIsSave ? "Save level as:" : "Load level:";
    DrawScreenText(title, cx - bw * 0.5f + 18.f, cy + 44.f, 0.9f, 0.9f, 1.f);

    float ibx = cx;
    float iby = cy + 4.f;
    DrawScreenRect(ibx, iby, bw - 36.f, 34.f, 0.08f, 0.08f, 0.10f, 1.f);
    DrawScreenRect(ibx, iby, bw - 36.f, 2.f, 0.4f, 0.7f, 1.f, 1.f);

    static float blinkT = 0.f;
    blinkT += (float)AEFrameRateControllerGetFrameTime();
    std::string display = gPromptInput;
    if ((int)(blinkT * 2.f) % 2 == 0) display += '|';

    DrawScreenText(display.c_str(),
        ibx - (bw - 36.f) * 0.5f + 10.f, iby - 8.f,
        1.f, 1.f, 1.f);

    if (!gPromptInput.empty())
    {
        std::string preview = NormaliseFilename(gPromptInput);
        const size_t maxChars = 52;
        if (preview.size() > maxChars)
            preview = "..." + preview.substr(preview.size() - maxChars);
        DrawScreenText(preview.c_str(),
            cx - bw * 0.5f + 18.f, cy - 46.f, 0.45f, 0.85f, 0.45f);
    }

    DrawScreenText("Enter to confirm  |  Press Z to cancel",
        cx - bw * 0.5f + 18.f, cy - 66.f, 0.5f, 0.5f, 0.55f);
}

/*========================================================
    play mode
========================================================*/

static void PlayMode_Enter()
{
    gPlayCamera = new Camera(
        { 0.f, 0.f },
        { (float)GRID_COLS, (float)GRID_ROWS },
        CAMERA_SCALE
    );

    gPlayEnemies = new EnemyManager();
    //gPlayBoss = new EnemyBoss();
    gPlayEnemies->SetBoss(gPlayBoss);
    std::vector<EnemyManager::SpawnInfo> spawns;
    bool hasBossSpawn = false;
    for (const auto& ed : gEnemyDefs)
    {
        spawns.push_back({ (EnemySpawnType)ed.preset, ed.pos });
        EnemySpawnType type = (EnemySpawnType)ed.preset;
        if (type == EnemySpawnType::Boss)
            hasBossSpawn = true;
    }
    if (hasBossSpawn)
    {
        gPlayBoss = new EnemyBoss();
        gPlayEnemies->SetBoss(gPlayBoss);
    }

    gPlayEnemies->SetSpawns(spawns);
    gPlayEnemies->SpawnAll();

    gPlayPlayer = new Player(gMap, gPlayEnemies);
    gPlayPlayer->Reset(gSpawn);
    UI::Init(gPlayPlayer);

    gPlayCamera->SetFollow(&gPlayPlayer->GetPosition(), 0, 0, true);

    gPlayTraps = new TrapManager();

    std::vector<PressurePlate*> spawnedPlates;
    std::vector<Trap*>          spawnedLinkTargets;

    for (const auto& td : gTrapDefs)
    {
        Box box{};
        box.size = td.size;
        box.position = AEVec2{
            td.pos.x - td.size.x * 0.5f,
            td.pos.y - td.size.y * 0.5f
        };

        if (td.type == (int)Trap::Type::SpikePlate)
        {
            SpikePlate& spikeRef = gPlayTraps->Spawn<SpikePlate>(
                box, td.upTime, td.downTime, td.damageOnHit, td.startDisabled);

            spawnedLinkTargets.push_back(&spikeRef);

            // parallel spike animation entry
            gSpikeTraps.push_back(&spikeRef);
            gSpikeAnims.emplace_back();
        }
        else if (td.type == (int)Trap::Type::PressurePlate)
        {
            PressurePlate& plateRef = gPlayTraps->Spawn<PressurePlate>(box);
            spawnedPlates.push_back(&plateRef);
        }
        else if (td.type == (int)Trap::Type::LavaPool)
        {
            gPlayTraps->Spawn<LavaPool>(box, td.damagePerTick, td.tickInterval);
        }
    }

    for (PressurePlate* plate : spawnedPlates)
        for (Trap* target : spawnedLinkTargets)
            plate->AddLinkedTrap(target);

}

static void PlayMode_Exit()
{
    UI::Exit();
    delete gPlayPlayer;  gPlayPlayer = nullptr;
    delete gPlayTraps;   gPlayTraps = nullptr;
    delete gPlayEnemies; gPlayEnemies = nullptr;
    delete gPlayCamera;  gPlayCamera = nullptr;
    delete gPlayBoss;   gPlayBoss = nullptr;
    gSpikeAnims.clear();
    gSpikeTraps.clear();

    ApplyWorldCamera();
}

static void PlayMode_Update(float dt)
{
    if (!gPlayPlayer || !gPlayCamera) return;

    gPlayPlayer->Update();
    gPlayCamera->Update();

    const AEVec2 pPos = gPlayPlayer->GetPosition();
    const AEVec2 pSize = gPlayPlayer->GetStats().playerSize;

    //gPlayEnemies->UpdateAll(pPos, *gMap);
    gPlayEnemies->UpdateAll(pPos, gPlayPlayer->IsFacingRight(), *gMap);

	attackSystem.UpdateEnemyAttack(*gPlayPlayer, *gPlayEnemies, gPlayBoss, *gMap);

    gPlayTraps->Update(dt, *gPlayPlayer);

    // tick spike animators
    // spike starts disabled (startDisabled=true), plate calls SetEnabled(true) — that's our trigger
    for (int i = 0; i < (int)gSpikeTraps.size(); ++i)
    {
        SpikeAnim& a = gSpikeAnims[i];

        if (!a.triggered && gSpikeTraps[i]->IsEnabled())
            a.triggered = true;

        if (!a.triggered || a.done) continue;

        a.timer += dt;
        if (a.timer >= 0.08f)
        {
            a.timer -= 0.08f;
            if (++a.frame >= 3) { a.frame = 3; a.done = true; }
        }
    }
    UI::GetDamageTextSpawner().Update();
    UI::Update();
}

static void PlayMode_Render()
{
    if (!gPlayPlayer || !gPlayCamera) return;

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    gMap->Render();

    // pressure plate green rects
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetColorToAdd(0, 0, 0, 0);
    for (const auto& td : gTrapDefs)
    {
        if (td.type != (int)Trap::Type::PressurePlate) continue;
        float wx = td.pos.x - td.size.x * 0.5f;
        float wy = td.pos.y - td.size.y * 0.5f;
        DrawWorldRect(wx, wy, td.size.x, td.size.y, 0.20f, 0.75f, 0.20f, 0.50f);
    }

    // spike animated sprites — draw black rect first to cover any external red outline
    int spikeIdx = 0;
    for (const auto& td : gTrapDefs)
    {
        if (td.type != (int)Trap::Type::SpikePlate) continue;

        const SpikeAnim& a = gSpikeAnims[spikeIdx++];
        float wx = td.pos.x - td.size.x * 0.5f;
        float wy = td.pos.y - td.size.y * 0.5f;

        // black rect to cover the red outline drawn externally
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToAdd(0.f, 0.f, 0.f, 0.f);
        DrawWorldRect(wx, wy, td.size.x, td.size.y, 0.f, 0.f, 0.f, 1.f);

        // spike sprite on top
        AEMtx33 m;
        AEMtx33Trans(&m, wx + 0.5f, wy + 0.5f);
        AEMtx33ScaleApply(&m, &m, Camera::scale, Camera::scale);
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1.f, 1.f, 1.f, 1.f);
        AEGfxSetColorToAdd(0.f, 0.f, 0.f, 0.f);
        AEGfxSetTransparency(1.f);
        AEGfxSetTransform(m.m);
        AEGfxTextureSet(gSpikeTexture, 0.f, 0.f);
        AEGfxMeshDraw(gSpikeMeshes[a.frame], AE_GFX_MDM_TRIANGLES);
    }

    gPlayPlayer->Render();
    gPlayEnemies->RenderAll();
    if (gPlayBoss)
        gPlayBoss->Render();

    // vine decorations (purely visual, same in both modes)
    if (gVineTexture && gVineMesh)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1.f, 1.f, 1.f, 1.f);
        AEGfxSetColorToAdd(0.f, 0.f, 0.f, 0.f);
        AEGfxSetTransparency(1.f);
        AEGfxTextureSet(gVineTexture, 0.f, 0.f);
        for (const auto& v : gVinePositions)
        {
            AEMtx33 m;
            AEMtx33Trans(&m, v.x + 0.5f, v.y + 0.5f);
            AEMtx33ScaleApply(&m, &m, Camera::scale, Camera::scale);
            AEGfxSetTransform(m.m);
            AEGfxMeshDraw(gVineMesh, AE_GFX_MDM_TRIANGLES);
        }
    }
    UI::Render();
}

/*========================================================
    editor update
========================================================*/

static void UpdateEditor(float dt)
{
    if (!gMap || !gCamera) return;

    if (!gUIIO.mouseCaptured)
    {
        if (AEInputCheckCurr(AEVK_W)) gCamera->position.y += CAMERA_SPEED * dt;
        if (AEInputCheckCurr(AEVK_S)) gCamera->position.y -= CAMERA_SPEED * dt;
        if (AEInputCheckCurr(AEVK_A)) gCamera->position.x -= CAMERA_SPEED * dt;
        if (AEInputCheckCurr(AEVK_D)) gCamera->position.x += CAMERA_SPEED * dt;

        s32 scroll = 0;
        AEInputMouseWheelDelta(&scroll);
        if (scroll != 0)
        {
            float factor = 1.0f + (float)scroll * ZOOM_SPEED;
            Camera::scale *= factor;
            if (Camera::scale < ZOOM_MIN) Camera::scale = ZOOM_MIN;
            if (Camera::scale > ZOOM_MAX) Camera::scale = ZOOM_MAX;
        }
    }

    ApplyWorldCamera();
    if (gUIIO.mouseCaptured || gPromptActive) return;

    s32 mx, my;
    AEInputGetCursorPosition(&mx, &my);
    AEVec2 world{};
    AEExtras::ScreenToWorldPosition(AEVec2{ (f32)mx, (f32)my }, world);

    int tx = (int)std::floor(world.x);
    int ty = (int)std::floor(world.y);
    gHoverCellValid = InBounds(tx, ty);
    gHoverCellX = tx;
    gHoverCellY = ty;

    if (!InBounds(tx, ty)) return;

    bool lmb = gUI.dragPaint ? AEInputCheckCurr(AEVK_LBUTTON)
        : AEInputCheckTriggered(AEVK_LBUTTON);
    bool rmb = AEInputCheckTriggered(AEVK_RBUTTON);

    if (rmb)
    {
        gMap->SetTile(tx, ty, MapTile::Type::NONE);
        RemoveTrapsAtCell(tx, ty);
        RemoveEnemyAtCell(tx, ty);
        // remove vine at this cell if present
        gVinePositions.erase(
            std::remove_if(gVinePositions.begin(), gVinePositions.end(),
                [tx, ty](const AEVec2& v) {
                    return (int)v.x == tx && (int)v.y == ty;
                }),
            gVinePositions.end());
        return;
    }
    if (!lmb) return;

    if (gUI.tool == EditorTool::Erase)
    {
        gMap->SetTile(tx, ty, MapTile::Type::NONE);
        RemoveTrapsAtCell(tx, ty);
        RemoveEnemyAtCell(tx, ty);
        gVinePositions.erase(
            std::remove_if(gVinePositions.begin(), gVinePositions.end(),
                [tx, ty](const AEVec2& v) {
                    return (int)v.x == tx && (int)v.y == ty;
                }),
            gVinePositions.end());
        return;
    }

    switch (gUI.brush)
    {
    case EditorTile::GroundSurface:
        gMap->SetTile(tx, ty, MapTile::Type::GROUND_SURFACE);
        break;
    case EditorTile::GroundBody:
        gMap->SetTile(tx, ty, MapTile::Type::GROUND_BODY);
        break;
    case EditorTile::GroundBottom:
        gMap->SetTile(tx, ty, MapTile::Type::GROUND_BOTTOM);
        break;
    case EditorTile::Platform:
        gMap->SetTile(tx, ty, MapTile::Type::PLATFORM);
        break;
    case EditorTile::Spike:
        gMap->SetTile(tx, ty, MapTile::Type::NONE);
        PlaceTrapAtCell(tx, ty, Trap::Type::SpikePlate);
        break;
    case EditorTile::PressurePlate:
        gMap->SetTile(tx, ty, MapTile::Type::GROUND_SURFACE);
        PlaceTrapAtCell(tx, ty, Trap::Type::PressurePlate);
        break;
    case EditorTile::Enemy:
    {
        EnemySpawnType preset = EnemySpawnType::Druid;

        switch (gUI.enemyPreset)
        {
        case EditorEnemyPreset::Skeleton:
            preset = EnemySpawnType::Skeleton;
            break;

        case EditorEnemyPreset::Boss:
            preset = EnemySpawnType::Boss;
            break;

        case EditorEnemyPreset::Druid:
        default:
            preset = EnemySpawnType::Druid;
            break;
        }

        PlaceEnemyAtCell(tx, ty, preset);
        break;
    }
    case EditorTile::Spawn:
        gSpawn = AEVec2{ (float)tx + 0.5f, (float)ty + 0.5f };
        break;
    case EditorTile::Vine:
    {
        // only add if not already present at this cell
        bool exists = false;
        for (const auto& v : gVinePositions)
            if ((int)v.x == tx && (int)v.y == ty) { exists = true; break; }
        if (!exists)
        {
            gVinePositions.push_back(AEVec2{ (float)tx, (float)ty });
            std::cout << "[Vine] placed at (" << tx << ", " << ty << ") total=" << gVinePositions.size() << "\n";
        }
        break;
    }
    }  // end switch
}  // end UpdateEditor

/*========================================================
    GameState lifecycle
========================================================*/

void GameState_LevelEditor_Load()
{
    int fontId = AEGfxCreateFont("Assets/buggy-font.ttf", 14);
    if (fontId < 0) fontId = AEGfxCreateFont("../Assets/buggy-font.ttf", 14);
    if (fontId < 0) fontId = AEGfxCreateFont("../../Assets/buggy-font.ttf", 14);
    gUIFont = static_cast<s8>(fontId);
}

void GameState_LevelEditor_Init()
{
    if (!gMap)
        gMap = new MapGrid(GRID_ROWS, GRID_COLS);

    if (!gCamera)
        gCamera = new Camera(
            { 0.f, 0.f },
            { (float)GRID_COLS, (float)GRID_ROWS },
            CAMERA_SCALE
        );

    gCamera->position = { GRID_COLS * 0.5f, GRID_ROWS * 0.5f };
    ApplyWorldCamera();

    EditorUI_Init();
    EditorUI_SetFont(gUIFont);
    OverlayInit();

    // load spike spritesheet and build per-frame meshes
    gSpikeTexture = AEGfxTextureLoad("Assets/Tmp/spikes.png");
    for (int i = 0; i < 4; ++i)
        gSpikeMeshes[i] = MakeSpikeMesh(i);

    // load vine texture and build simple unit quad
    gVineTexture = AEGfxTextureLoad("Assets/Tmp/vines.png");
    std::cout << "[Vine] texture load: " << (gVineTexture ? "OK" : "FAILED - check Assets/Tmp/vines.png") << "\n";
    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.f, 1.f,
        0.5f, -0.5f, 0xFFFFFFFF, 1.f, 1.f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.f, 0.f);
    AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.f, 1.f,
        0.5f, 0.5f, 0xFFFFFFFF, 1.f, 0.f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.f, 0.f);
    gVineMesh = AEGfxMeshEnd();

    gUI = EditorUIState{};
    gUIIO = EditorUIIO{};

    gTrapDefs.clear();
    gEnemyDefs.clear();
    gSpawn = { 5.f, 5.f };

    gPromptActive = false;
    gPromptInput = "";
    gSaveMessageTimer = 0.f;
}

void GameState_LevelEditor_Update()
{
    if (!gMap || !gCamera) return;

    const float dt = (float)AEFrameRateControllerGetFrameTime();

    if (gPromptActive)
    {
        Prompt_Update();
        if (gSaveMessageTimer > 0.f) gSaveMessageTimer -= dt;
        return;
    }

    if (gUI.requestSave) { Prompt_Open(true);  gUI.requestSave = false; }
    if (gUI.requestLoad) { Prompt_Open(false); gUI.requestLoad = false; }

    if (gUI.requestClearMap)
    {
        for (int row = 0; row < GRID_ROWS; ++row)
            for (int col = 0; col < GRID_COLS; ++col)
                gMap->SetTile(col, row, MapTile::Type::NONE);
        gTrapDefs.clear();
        gEnemyDefs.clear();
        gVinePositions.clear();
        gUI.requestClearMap = false;
    }

    if (gSaveMessageTimer > 0.f) gSaveMessageTimer -= dt;

    bool prevPlayMode = gUI.playMode;

    if (gUI.requestTogglePlay)
    {
        gUI.playMode = !gUI.playMode;
        gUI.requestTogglePlay = false;
    }
    if (AEInputCheckTriggered(AEVK_TAB))
        gUI.playMode = !gUI.playMode;

    if (gUI.playMode != prevPlayMode)
    {
        if (gUI.playMode) PlayMode_Enter();
        else              PlayMode_Exit();
    }

    if (gUI.playMode)
        PlayMode_Update(dt);
    else
        UpdateEditor(dt);
}

void GameState_LevelEditor_Draw()
{
    if (!gMap || !gCamera) return;

    AEGfxSetBackgroundColor(0.129f, 0.114f, 0.18f);

    AEMtx33 identity;
    AEMtx33Identity(&identity);
    AEGfxSetTransform(identity.m);

    if (gUI.playMode)
    {
        PlayMode_Render();
    }
    else
    {
        ApplyWorldCamera();

        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        gMap->Render();

        if (gUI.showGrid)
            DrawGridOverlay();

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetColorToAdd(0, 0, 0, 0);
        for (const auto& t : gTrapDefs)
        {
            float wx = t.pos.x - t.size.x * 0.5f;
            float wy = t.pos.y - t.size.y * 0.5f;

            if (t.type == (int)Trap::Type::SpikePlate)
            {
                AEMtx33 m;
                AEMtx33Trans(&m, wx + 0.5f, wy + 0.5f);
                AEMtx33ScaleApply(&m, &m, Camera::scale, Camera::scale);
                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(1.f, 1.f, 1.f, 1.f);
                AEGfxSetColorToAdd(0.f, 0.f, 0.f, 0.f);
                AEGfxSetTransparency(1.f);
                AEGfxSetTransform(m.m);
                AEGfxTextureSet(gSpikeTexture, 0.f, 0.f);
                AEGfxMeshDraw(gSpikeMeshes[3], AE_GFX_MDM_TRIANGLES);
            }
            else if (t.type == (int)Trap::Type::PressurePlate)
            {
                AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                DrawWorldRect(wx, wy, t.size.x, t.size.y, 0.20f, 0.75f, 0.20f, 0.70f);
            }
        }

        for (const auto& ed : gEnemyDefs)
        {
            float wx = ed.pos.x - 0.5f;
            float wy = ed.pos.y - 0.5f;
            bool isSkeleton = (ed.preset == (int)EnemySpawnType::Skeleton);
            float r = isSkeleton ? 0.6f : 1.0f;
            float g = isSkeleton ? 0.6f : 0.85f;
            float b = isSkeleton ? 1.0f : 0.0f;
            DrawWorldRect(wx, wy, 1.f, 1.f, r, g, b, 0.70f);
        }

        DrawWorldRect(gSpawn.x - 0.15f, gSpawn.y - 0.15f, 0.3f, 0.3f, 1, 1, 1, 1);

        // vine decorations
        if (gVineTexture && gVineMesh)
        {
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1.f, 1.f, 1.f, 1.f);
            AEGfxSetColorToAdd(0.f, 0.f, 0.f, 0.f);
            AEGfxSetTransparency(1.f);
            AEGfxTextureSet(gVineTexture, 0.f, 0.f);
            for (const auto& v : gVinePositions)
            {
                AEMtx33 m;
                AEMtx33Trans(&m, v.x + 0.5f, v.y + 0.5f);
                AEMtx33ScaleApply(&m, &m, Camera::scale, Camera::scale);
                AEGfxSetTransform(m.m);
                AEGfxMeshDraw(gVineMesh, AE_GFX_MDM_TRIANGLES);
            }
        }
    }

    s32 mx, my;
    AEInputGetCursorPosition(&mx, &my);

    EditorUI_Draw(
        gUI, gUIIO,
        (int)AEGfxGetWindowWidth(),
        (int)AEGfxGetWindowHeight(),
        mx, my,
        AEInputCheckTriggered(AEVK_LBUTTON)
    );

    if (gUIFont >= 0 && gHoverCellValid)
    {
        char buf[64];
        sprintf_s(buf, "cell: (%d, %d)", gHoverCellX, gHoverCellY);
        DrawScreenText(buf, 260.0f, 16.0f, 1.0f, 1.0f, 1.0f);
    }

    Prompt_Draw();

    if (gSaveMessageTimer > 0.f)
    {
        const char* msg = gSaveSuccess ? "saved!" : "failed!";
        float r = gSaveSuccess ? 0.2f : 1.0f;
        float g = gSaveSuccess ? 1.0f : 0.2f;
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetColorToAdd(0, 0, 0, 0);
        AEGfxSetColorToMultiply(1, 1, 1, 1);
        AEGfxSetTransparency(1.f);
        AEGfxPrint(gUIFont, msg, 0.1f, 0.85f, 1.0f, r, g, 0.2f, 1.0f);
    }

    ApplyWorldCamera();
    AEMtx33Identity(&identity);
    AEGfxSetTransform(identity.m);
}

void GameState_LevelEditor_Free()
{
    if (gUI.playMode) PlayMode_Exit();

    OverlayShutdown();

    if (gSpikeTexture) { AEGfxTextureUnload(gSpikeTexture); gSpikeTexture = nullptr; }
    for (int i = 0; i < 4; ++i)
        if (gSpikeMeshes[i]) { AEGfxMeshFree(gSpikeMeshes[i]); gSpikeMeshes[i] = nullptr; }

    if (gVineTexture) { AEGfxTextureUnload(gVineTexture); gVineTexture = nullptr; }
    if (gVineMesh) { AEGfxMeshFree(gVineMesh);         gVineMesh = nullptr; }
    gVinePositions.clear();

    delete gMap;    gMap = nullptr;
    delete gCamera; gCamera = nullptr;
    gTrapDefs.clear();
    gEnemyDefs.clear();
}

void GameState_LevelEditor_Unload()
{
    EditorUI_Shutdown();
    if (gUIFont >= 0)
    {
        AEGfxDestroyFont(gUIFont);
        gUIFont = -1;
    }
}