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
#include "../Time.h"

#include <iostream>
#include <vector>
#include <cmath>
#include <cstdio>
#include <string>

/*========================================================
    configuration
========================================================*/

static constexpr int   GRID_ROWS = 50;
static constexpr int   GRID_COLS = 100;
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

/*========================================================
    save prompt state
========================================================*/

static bool        gPromptActive = false;
static bool        gPromptIsSave = true;   // true=save, false=load
static std::string gPromptInput = "";
static bool        gPromptJustOpened = false;



// Returns absolute path: <exe_dir>/Assets/Levels/<name>.lvl
// Also ensures the directory exists.
static std::string NormaliseFilename(const std::string& name)
{
    // Get directory of the running executable
    char exePath[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);

    // Strip the exe filename to get the directory
    std::string dir(exePath);
    size_t slash = dir.find_last_of("\\/");
    if (slash != std::string::npos)
        dir = dir.substr(0, slash + 1); // includes trailing slash
    else
        dir = ".\\";

    // Append subfolder
    std::string folder = dir + "Assets\\Levels\\";

    // Create directories if they don't exist
    CreateDirectoryA((dir + "Assets").c_str(), nullptr);
    CreateDirectoryA(folder.c_str(), nullptr);

    // Append .lvl if needed
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

/*========================================================
    helpers
========================================================*/

static bool InBounds(int x, int y)
{
    return x >= 0 && x < GRID_COLS && y >= 0 && y < GRID_ROWS;
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

static void PlaceEnemyAtCell(int tx, int ty, Enemy::Preset preset)
{
    RemoveEnemyAtCell(tx, ty);
    EnemyDefSimple e{};
    e.preset = (int)preset;
    e.pos = AEVec2{ tx + 0.5f, ty + 0.f };
    gEnemyDefs.push_back(e);
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

// Draw a screen-space rect (pixel coords, origin bottom-left, y-up)
static void DrawScreenRect(float cx, float cy, float w, float h,
    float r, float g, float b, float a)
{
    // enforce UI camera
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

    // Skip input on the frame we opened
    if (gPromptJustOpened) { gPromptJustOpened = false; return; }

    // AEInputCheckTriggered = curr && !prev — exactly one fire per keypress
    bool shift = AEInputCheckCurr(AEVK_LSHIFT) || AEInputCheckCurr(AEVK_RSHIFT);

    // Letters A-Z  (VK codes 0x41-0x5A match AEVK_A-AEVK_Z)
    for (u8 vk = AEVK_A; vk <= AEVK_Z; ++vk)
    {
        if (AEInputCheckTriggered(vk) && gPromptInput.size() < 32)
        {
            char c = shift ? (char)vk : (char)(vk + 32);
            gPromptInput += c;
        }
    }

    // Digits 0-9  (VK codes 0x30-0x39 match AEVK_0-AEVK_9)
    for (u8 vk = AEVK_0; vk <= AEVK_9; ++vk)
    {
        if (AEInputCheckTriggered(vk) && gPromptInput.size() < 32)
            gPromptInput += (char)vk;
    }

    // Hyphen / underscore
    if (AEInputCheckTriggered(AEVK_MINUS) && gPromptInput.size() < 32)
        gPromptInput += shift ? '_' : '-';

    // Backspace
    if (AEInputCheckTriggered(AEVK_BACK) && !gPromptInput.empty())
        gPromptInput.pop_back();

    // Confirm
    if (AEInputCheckTriggered(AEVK_RETURN) && !gPromptInput.empty())
    {
        std::string path = NormaliseFilename(gPromptInput);

        if (gPromptIsSave)
        {
            LevelData lvl;
            BuildLevelDataFromEditor(*gMap, GRID_ROWS, GRID_COLS,
                gTrapDefs, gEnemyDefs, gSpawn, lvl);
            gSaveSuccess = SaveLevelToFile(path.c_str(), lvl);
            gSaveMessageTimer = 2.5f;
        }
        else
        {
            LevelData lvl;
            if (LoadLevelFromFile(path.c_str(), lvl))
            {
                ApplyLevelDataToEditor(lvl, gMap, gTrapDefs, gEnemyDefs, gSpawn);
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

    // Cancel
    if (AEInputCheckTriggered(AEVK_ESCAPE))
        gPromptActive = false;
}

static void Prompt_Draw()
{
    if (!gPromptActive) return;

    int winW = AEGfxGetWindowWidth();
    int winH = AEGfxGetWindowHeight();

    float cx = winW * 0.5f;
    float cy = winH * 0.5f;

    // dimmed backdrop
    DrawScreenRect(cx, cy, (float)winW, (float)winH, 0.f, 0.f, 0.f, 0.55f);

    // dialog box
    float bw = 420.f, bh = 160.f;
    DrawScreenRect(cx, cy, bw, bh, 0.15f, 0.15f, 0.18f, 0.97f);

    // border
    DrawScreenRect(cx, cy, bw, 2.f, 0.4f, 0.7f, 1.f, 1.f);
    DrawScreenRect(cx, cy + bh * 0.5f - 1.f, bw, 2.f, 0.4f, 0.7f, 1.f, 1.f);

    const char* title = gPromptIsSave ? "Save level as:" : "Load level:";
    DrawScreenText(title, cx - bw * 0.5f + 18.f, cy + 44.f, 0.9f, 0.9f, 1.f);

    // input box background
    float ibx = cx;
    float iby = cy + 4.f;
    DrawScreenRect(ibx, iby, bw - 36.f, 34.f, 0.08f, 0.08f, 0.10f, 1.f);
    DrawScreenRect(ibx, iby, bw - 36.f, 2.f, 0.4f, 0.7f, 1.f, 1.f);

    // input text + cursor blink
    static float blinkT = 0.f;
    blinkT += (float)AEFrameRateControllerGetFrameTime();
    std::string display = gPromptInput;
    if ((int)(blinkT * 2.f) % 2 == 0) display += '|';

    DrawScreenText(display.c_str(),
        ibx - (bw - 36.f) * 0.5f + 10.f,
        iby - 8.f,
        1.f, 1.f, 1.f);

    // show resolved path so user knows exactly where file will be saved
    if (!gPromptInput.empty())
    {
        std::string preview = NormaliseFilename(gPromptInput);
        // truncate from left if too long to fit in dialog
        const size_t maxChars = 52;
        if (preview.size() > maxChars)
            preview = "..." + preview.substr(preview.size() - maxChars);
        DrawScreenText(preview.c_str(),
            cx - bw * 0.5f + 18.f, cy - 46.f, 0.45f, 0.85f, 0.45f);
    }

    // hint
    DrawScreenText("Enter to confirm  |  Esc to cancel",
        cx - bw * 0.5f + 18.f, cy - 66.f, 0.5f, 0.5f, 0.55f);
}

/*========================================================
    play mode
========================================================*/

static void PlayMode_Enter()
{
    // --- camera ---
    gPlayCamera = new Camera(
        { 0.f, 0.f },
        { (float)GRID_COLS, (float)GRID_ROWS },
        CAMERA_SCALE
    );
    gPlayCamera->position = gSpawn;

    // --- player ---
    gPlayPlayer = new Player(gMap, nullptr);
    gPlayPlayer->Reset(gSpawn);

    // --- traps ---
    gPlayTraps = new TrapManager();

    // Collect pointers so we can link pressure plates after all traps are spawned
    std::vector<PressurePlate*> spawnedPlates;
    std::vector<Trap*>          spawnedLinkTargets; // currently: all spike plates

    for (const auto& td : gTrapDefs)
    {
        // traps.cpp currently treats Box.position as min corner (left/bottom)
        // editor stores td.pos as center, so convert center -> min corner
        Box box{};
        box.size = td.size;
        box.position = AEVec2{
            td.pos.x - td.size.x * 0.5f,
            td.pos.y - td.size.y * 0.5f
        };

        if (td.type == (int)Trap::Type::SpikePlate)
        {
            SpikePlate& spikeRef = gPlayTraps->Spawn<SpikePlate>(
                box,
                td.upTime,
                td.downTime,
                td.damageOnHit,
                td.startDisabled
            );

            spawnedLinkTargets.push_back(&spikeRef);
        }
        else if (td.type == (int)Trap::Type::PressurePlate)
        {
            PressurePlate& plateRef = gPlayTraps->Spawn<PressurePlate>(box);
            spawnedPlates.push_back(&plateRef);
        }
        else if (td.type == (int)Trap::Type::LavaPool)
        {
            // If your TrapDefSimple doesn't have these fields, replace with defaults.
            gPlayTraps->Spawn<LavaPool>(box, td.damagePerTick, td.tickInterval);
        }
    }

    // Temporary linking rule: every pressure plate controls every spike plate
    for (PressurePlate* plate : spawnedPlates)
    {
        for (Trap* target : spawnedLinkTargets)
        {
            plate->AddLinkedTrap(target);
        }
    }

    // --- enemies ---
    gPlayEnemies = new EnemyManager();
    std::vector<EnemyManager::SpawnInfo> spawns;
    for (const auto& ed : gEnemyDefs)
        spawns.push_back({ (Enemy::Preset)ed.preset, ed.pos });
    gPlayEnemies->SetSpawns(spawns);
    gPlayEnemies->SpawnAll();
}



static void PlayMode_Exit()
{
    delete gPlayPlayer;  gPlayPlayer = nullptr;
    delete gPlayTraps;   gPlayTraps = nullptr;
    delete gPlayEnemies; gPlayEnemies = nullptr;
    delete gPlayCamera;  gPlayCamera = nullptr;
    ApplyWorldCamera();
}

static void PlayMode_Update(float dt)
{
    if (!gPlayPlayer || !gPlayCamera) return;

    gPlayPlayer->Update();
    // manually track player — no SetFollow needed
    gPlayCamera->position = gPlayPlayer->GetPosition();

    const AEVec2 pPos = gPlayPlayer->GetPosition();
    const AEVec2 pSize = gPlayPlayer->GetStats().playerSize;

    gPlayEnemies->UpdateAll(pPos, *gMap);

    gPlayEnemies->ForEachEnemy([&](Enemy& e)
        {
            if (!e.PollAttackHit()) return;
            const AEVec2 ePos = e.GetPosition();
            if (std::fabs(pPos.x - ePos.x) <= e.GetAttackHitRange() &&
                std::fabs(pPos.y - ePos.y) <= pSize.y * 0.5f + 0.6f)
                gPlayPlayer->TakeDamage(e.GetAttackDamage(), e.GetPosition());
        });

    gPlayTraps->Update(dt, *gPlayPlayer);
}

static void PlayMode_Render()
{
    if (!gPlayPlayer || !gPlayCamera) return;

    // apply play camera
    AEGfxSetCamPosition(
        gPlayCamera->position.x * Camera::scale,
        gPlayCamera->position.y * Camera::scale
    );
    Camera::position = gPlayCamera->position;

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    gMap->Render();

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetColorToAdd(0, 0, 0, 0);
    for (const auto& td : gTrapDefs)
    {
        float wx = td.pos.x - td.size.x * 0.5f;
        float wy = td.pos.y - td.size.y * 0.5f;
        float r, g, b;
        if (td.type == (int)Trap::Type::SpikePlate) { r = 0.85f; g = 0.20f; b = 0.20f; }
        else if (td.type == (int)Trap::Type::PressurePlate) { r = 0.20f; g = 0.75f; b = 0.20f; }
        else continue;
        DrawWorldRect(wx, wy, td.size.x, td.size.y, r, g, b, 0.50f);
    }

    gPlayPlayer->Render();
    gPlayEnemies->RenderAll();
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
    if (!InBounds(tx, ty)) return;

    bool lmb = gUI.dragPaint ? AEInputCheckCurr(AEVK_LBUTTON)
        : AEInputCheckTriggered(AEVK_LBUTTON);
    bool rmb = AEInputCheckTriggered(AEVK_RBUTTON);

    if (rmb)
    {
        gMap->SetTile(tx, ty, MapTile::Type::NONE);
        RemoveTrapsAtCell(tx, ty);
        RemoveEnemyAtCell(tx, ty);
        return;
    }
    if (!lmb) return;

    if (gUI.tool == EditorTool::Erase)
    {
        gMap->SetTile(tx, ty, MapTile::Type::NONE);
        RemoveTrapsAtCell(tx, ty);
        RemoveEnemyAtCell(tx, ty);
        return;
    }

    switch (gUI.brush)
    {
    case EditorTile::Ground:
        gMap->SetTile(tx, ty, MapTile::Type::GROUND);
        break;
    case EditorTile::Spike:
        gMap->SetTile(tx, ty, MapTile::Type::GROUND);
        PlaceTrapAtCell(tx, ty, Trap::Type::SpikePlate);
        break;
    case EditorTile::PressurePlate:
        gMap->SetTile(tx, ty, MapTile::Type::GROUND);
        PlaceTrapAtCell(tx, ty, Trap::Type::PressurePlate);
        break;
    case EditorTile::Enemy:
    {
        Enemy::Preset preset = (gUI.enemyPreset == EditorEnemyPreset::Skeleton)
            ? Enemy::Preset::Skeleton : Enemy::Preset::Druid;
        PlaceEnemyAtCell(tx, ty, preset);
        break;
    }
    }
}

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

    // ── prompt takes priority over everything else ─────────────────────────
    if (gPromptActive)
    {
        Prompt_Update();
        if (gSaveMessageTimer > 0.f) gSaveMessageTimer -= dt;
        return;
    }

    // ── UI one-frame requests ─────────────────────────────────────────────
    if (gUI.requestSave)
    {
        Prompt_Open(true);
        gUI.requestSave = false;
    }

    if (gUI.requestLoad)
    {
        Prompt_Open(false);
        gUI.requestLoad = false;
    }

    if (gUI.requestClearMap)
    {
        for (int row = 0; row < GRID_ROWS; ++row)
            for (int col = 0; col < GRID_COLS; ++col)
                gMap->SetTile(col, row, MapTile::Type::NONE);
        gTrapDefs.clear();
        gEnemyDefs.clear();
        gUI.requestClearMap = false;
    }

    if (gSaveMessageTimer > 0.f) gSaveMessageTimer -= dt;

    // ── play mode toggle ──────────────────────────────────────────────────
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

    // ── tick ─────────────────────────────────────────────────────────────
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

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetColorToAdd(0, 0, 0, 0);
        for (const auto& t : gTrapDefs)
        {
            float wx = t.pos.x - t.size.x * 0.5f;
            float wy = t.pos.y - t.size.y * 0.5f;
            float r, g, b;
            if (t.type == (int)Trap::Type::SpikePlate) { r = 0.85f; g = 0.20f; b = 0.20f; }
            else if (t.type == (int)Trap::Type::PressurePlate) { r = 0.20f; g = 0.75f; b = 0.20f; }
            else continue;
            DrawWorldRect(wx, wy, t.size.x, t.size.y, r, g, b, 0.70f);
        }

        for (const auto& ed : gEnemyDefs)
        {
            float wx = ed.pos.x - 0.5f;
            float wy = ed.pos.y - 0.5f;
            bool isSkeleton = (ed.preset == (int)Enemy::Preset::Skeleton);
            float r = isSkeleton ? 0.6f : 1.0f;
            float g = isSkeleton ? 0.6f : 0.85f;
            float b = isSkeleton ? 1.0f : 0.0f;
            DrawWorldRect(wx, wy, 1.f, 1.f, r, g, b, 0.70f);
        }

        // spawn point
        DrawWorldRect(gSpawn.x - 0.15f, gSpawn.y - 0.15f, 0.3f, 0.3f, 1, 1, 1, 1);
    }

    // ── UI panel ─────────────────────────────────────────────────────────
    s32 mx, my;
    AEInputGetCursorPosition(&mx, &my);

    EditorUI_Draw(
        gUI, gUIIO,
        (int)AEGfxGetWindowWidth(),
        (int)AEGfxGetWindowHeight(),
        mx, my,
        AEInputCheckCurr(AEVK_LBUTTON)
    );

    // ── save prompt overlay (on top of everything) ────────────────────────
    Prompt_Draw();

    // ── save feedback message ─────────────────────────────────────────────
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

