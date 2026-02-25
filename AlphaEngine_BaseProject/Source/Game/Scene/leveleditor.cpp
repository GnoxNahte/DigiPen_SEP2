
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

#include <vector>
#include <cmath>

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

static const char* LEVEL_PATH = "level01.lvl";

/*========================================================
    editor state
========================================================*/

static MapGrid* gMap = nullptr;
static Camera* gCamera = nullptr;

static EditorUIState gUI{};
static EditorUIIO    gUIIO{};
static s8            gUIFont = -1;

static std::vector<TrapDefSimple>   gTrapDefs;
static std::vector<EnemyDefSimple>  gEnemyDefs;     // ✅ matches LevelIO
static AEVec2                       gSpawn{ 5.0f, 5.0f };

static float gSaveMessageTimer = 0.f;
static bool  gSaveSuccess = false;

/*========================================================
    play mode state
========================================================*/

static Player* gPlayPlayer = nullptr;
static TrapManager* gPlayTraps = nullptr;
static EnemyManager* gPlayEnemies = nullptr;
static Camera* gPlayCamera = nullptr;

static bool gInPlayMode = false; // authoritative runtime flag

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

    // default spike params (safe)
    t.upTime = 1.0f;
    t.downTime = 1.0f;
    t.damageOnHit = 10;
    t.startDisabled = false;

    for (auto& e : gTrapDefs)
    {
        int ex = (int)std::floor(e.pos.x);
        int ey = (int)std::floor(e.pos.y);
        if (ex == tx && ey == ty && e.type == t.type)
        {
            e = t;
            return;
        }
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
    e.preset = (int)preset; // ✅ stored as int for IO friendliness
    e.pos = AEVec2{ tx + 0.5f, ty + 0.9f};
    gEnemyDefs.push_back(e);
}

/*========================================================
    overlay quad
========================================================*/

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
    float sw = worldW * Camera::scale;
    float sh = worldH * Camera::scale;

    AEMtx33 sc, ro, tr, m;
    AEMtx33Rot(&ro, 0.f);
    AEMtx33Scale(&sc, sw, sh);
    AEMtx33Trans(&tr, cx, cy);
    AEMtx33Concat(&m, &ro, &sc);
    AEMtx33Concat(&m, &tr, &m);

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransform(m.m);
    AEGfxSetColorToMultiply(r, g, b, a);
    AEGfxMeshDraw(gOverlayQuad, AE_GFX_MDM_TRIANGLES);
}

/*========================================================
    play mode (forward decl + mode switch)
========================================================*/

static void PlayMode_Enter();
static void PlayMode_Exit();
static void PlayMode_Update(float dt);
static void PlayMode_Render();

static void SetPlayMode(bool enabled)
{
    if (enabled == gInPlayMode)
        return;

    gInPlayMode = enabled;
    gUI.playMode = enabled; // keep ui synced

    if (gInPlayMode) PlayMode_Enter();
    else             PlayMode_Exit();
}

/*========================================================
    play mode – lifecycle
========================================================*/

static void PlayMode_Enter()
{
    gPlayCamera = new Camera(
        { 0.f, 0.f },
        { (float)GRID_COLS, (float)GRID_ROWS },
        CAMERA_SCALE
    );

    gPlayPlayer = new Player(gMap, nullptr);
    gPlayPlayer->Reset(gSpawn);
    gPlayCamera->position = gSpawn;

    // traps
    gPlayTraps = new TrapManager();
    for (const auto& td : gTrapDefs)
    {
        Box box{ td.pos, td.size };

        if (td.type == (int)Trap::Type::SpikePlate)
        {
            gPlayTraps->Spawn<SpikePlate>(
                box,
                td.upTime, td.downTime,
                td.damageOnHit, td.startDisabled
            );
        }
        else if (td.type == (int)Trap::Type::PressurePlate)
        {
            gPlayTraps->Spawn<PressurePlate>(box);
        }
        else if (td.type == (int)Trap::Type::LavaPool)
        {
            gPlayTraps->Spawn<LavaPool>(box, td.damagePerTick, td.tickInterval);
        }
    }

    // enemies
    gPlayEnemies = new EnemyManager();
    std::vector<EnemyManager::SpawnInfo> spawns;
    spawns.reserve(gEnemyDefs.size());
    for (const auto& ed : gEnemyDefs)
    {
        AEVec2 pos = ed.pos;

        const int tx = (int)floorf(pos.x);
        int ty = (int)floorf(pos.y);

        // enemy collider size is 0.8, so half height is 0.4
        constexpr float enemyHalfH = 0.8f * 0.5f;

        // Find the first GROUND tile at-or-below the placed cell
        int groundY = -1;
        for (int y = ty; y >= 0; --y)
        {
            // Check tile at (tx, y) using world point at its center
            if (gMap->CheckPointCollision((float)tx + 0.5f, (float)y + 0.5f))
            {
                groundY = y;
                break;
            }
        }

        if (groundY >= 0)
        {
            // Put enemy on TOP of that ground tile
            pos.y = (float)groundY + 1.0f + enemyHalfH; // y + 1.4
        }
        spawns.push_back({ (Enemy::Preset)ed.preset, ed.pos });
    }
       

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
    (void)dt;

    if (!gPlayPlayer || !gPlayCamera) return;

    if (AEInputCheckTriggered(AEVK_ESCAPE))
    {
        SetPlayMode(false);
        return;
    }

    if (AEInputCheckTriggered(AEVK_R))
    {
        gPlayPlayer->Reset(gSpawn);
        gPlayCamera->position = gSpawn;
    }

    gPlayPlayer->Update();
    gPlayCamera->position = gPlayPlayer->GetPosition();

    const AEVec2 pPos = gPlayPlayer->GetPosition();
    gPlayEnemies->UpdateAll(pPos, *gMap);

    const AEVec2 pSize = gPlayPlayer->GetStats().playerSize;
    gPlayEnemies->ForEachEnemy([&](Enemy& e)
        {
            if (!e.PollAttackHit()) return;

            const AEVec2 ePos = e.GetPosition();
            const float dx = std::fabs(pPos.x - ePos.x);
            const float dy = std::fabs(pPos.y - ePos.y);

            if (dx <= e.GetAttackHitRange() && dy <= (pSize.y * 0.5f + 0.6f))
                gPlayPlayer->TakeDamage(e.GetAttackDamage(), e.GetPosition());
        });

    gPlayTraps->Update(dt, *gPlayPlayer);
}

static void PlayMode_Render()
{
    if (!gPlayPlayer || !gPlayCamera) return;

    AEGfxSetCamPosition(
        gPlayCamera->position.x * Camera::scale,
        gPlayCamera->position.y * Camera::scale
    );
    Camera::position = gPlayCamera->position;

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    gMap->Render();

    // optional trap overlay for visibility
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
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

    // camera movement (blocked when mouse on ui)
    if (!gUIIO.mouseCaptured)
    {
        if (AEInputCheckCurr(AEVK_W)) gCamera->position.y += CAMERA_SPEED * dt;
        if (AEInputCheckCurr(AEVK_S)) gCamera->position.y -= CAMERA_SPEED * dt;
        if (AEInputCheckCurr(AEVK_A)) gCamera->position.x -= CAMERA_SPEED * dt;
        if (AEInputCheckCurr(AEVK_D)) gCamera->position.x += CAMERA_SPEED * dt;
    }

    // zoom: do NOT depend on gUIIO.mouseCaptured (can be stale in update)
    // instead, block zoom when cursor is over left ui panel.
    s32 mx = 0, my = 0;
    AEInputGetCursorPosition(&mx, &my);
    const bool overUIPanel = (mx >= 0 && mx < (s32)gUI.panelW);

    if (!overUIPanel)
    {
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

    // don’t paint if mouse is in ui panel
    if (gUIIO.mouseCaptured) return;

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

    // paint
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
        Enemy::Preset preset =
            (gUI.enemyPreset == EditorEnemyPreset::Skeleton)
            ? Enemy::Preset::Skeleton
            : Enemy::Preset::Druid;

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

    gUIFont = (s8)fontId;
    if (gUIFont < 0) OutputDebugStringA("EditorUI: font load failed\n");
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
    Camera::scale = CAMERA_SCALE;
    ApplyWorldCamera();

    EditorUI_Init();
    EditorUI_SetFont(gUIFont);
    OverlayInit();

    gUI = EditorUIState{};
    gUIIO = EditorUIIO{};

    gInPlayMode = false;
    gUI.playMode = false;

    gTrapDefs.clear();
    gEnemyDefs.clear();
    gSpawn = { 5.f, 5.f };
}

void GameState_LevelEditor_Update()
{
    if (!gMap || !gCamera) return;

    const float dt = (float)AEFrameRateControllerGetFrameTime();

    // tab toggles play mode too
    if (AEInputCheckTriggered(AEVK_TAB))
        SetPlayMode(!gInPlayMode);

    // ui requested toggle
    if (gUI.requestTogglePlay)
    {
        SetPlayMode(!gInPlayMode);
        gUI.requestTogglePlay = false;
    }

    // safety sync
    if (gUI.playMode != gInPlayMode)
        SetPlayMode(gUI.playMode);

    // save / load / clear (editor only)
    if (!gInPlayMode && gUI.requestSave)
    {
        LevelData lvl;
        BuildLevelDataFromEditor(*gMap, GRID_ROWS, GRID_COLS, gTrapDefs, gEnemyDefs, gSpawn, lvl);
        gSaveSuccess = SaveLevelToFile(LEVEL_PATH, lvl);
        gSaveMessageTimer = 2.5f;
        gUI.requestSave = false;
        OutputDebugStringA(gSaveSuccess ? "SAVE OK\n" : "SAVE FAILED\n");
    }

    if (!gInPlayMode && gUI.requestLoad)
    {
        LevelData lvl;
        if (LoadLevelFromFile(LEVEL_PATH, lvl))
            ApplyLevelDataToEditor(lvl, gMap, gTrapDefs, gEnemyDefs, gSpawn);

        gUI.requestLoad = false;
    }

    if (!gInPlayMode && gUI.requestClearMap)
    {
        for (int row = 0; row < GRID_ROWS; ++row)
            for (int col = 0; col < GRID_COLS; ++col)
                gMap->SetTile(col, row, MapTile::Type::NONE);

        gTrapDefs.clear();
        gEnemyDefs.clear();
        gSpawn = { 5.f, 5.f };
        gUI.requestClearMap = false;
    }

    if (gSaveMessageTimer > 0.f)
        gSaveMessageTimer -= dt;

    if (gInPlayMode) PlayMode_Update(dt);
    else             UpdateEditor(dt);
}

void GameState_LevelEditor_Draw()
{
    if (!gMap || !gCamera) return;

    AEGfxSetBackgroundColor(0.10f, 0.10f, 0.10f);

    AEMtx33 identity;
    AEMtx33Identity(&identity);
    AEGfxSetTransform(identity.m);

    // ── world ──────────────────────────────────────────────────────────────
    if (gInPlayMode)
    {
        PlayMode_Render(); // ✅ prevents C4505 (/WX) unused static warning
    }
    else
    {
        ApplyWorldCamera();

        // map
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        gMap->Render();

        // overlays
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToAdd(0, 0, 0, 0);

        // traps
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

        // enemy spawns
        for (const auto& ed : gEnemyDefs)
        {
            float wx = ed.pos.x - 0.5f;
            float wy = ed.pos.y - 0.5f;

            float r = 1.0f, g = 0.85f, b = 0.0f; // yellow default

            // ✅ preset stored as int
            if (ed.preset == (int)Enemy::Preset::Skeleton)
            {
                r = 0.6f; g = 0.6f; b = 1.0f; // blue for skeleton
            }

            DrawWorldRect(wx, wy, 1.f, 1.f, r, g, b, 0.70f);
        }

        // spawn marker
        DrawWorldRect(gSpawn.x - 0.15f, gSpawn.y - 0.15f, 0.3f, 0.3f, 1, 1, 1, 1);
    }

    // ── ui ─────────────────────────────────────────────────────────────────
    s32 mx, my;
    AEInputGetCursorPosition(&mx, &my);

    EditorUI_Draw(
        gUI, gUIIO,
        (int)AEGfxGetWindowWidth(),
        (int)AEGfxGetWindowHeight(),
        mx, my,
        AEInputCheckTriggered(AEVK_LBUTTON) // ✅ fixes play/stop lag (no spam)
    );

    // ── save feedback ───────────────────────────────────────────────────────
    if (gSaveMessageTimer > 0.f && gUIFont >= 0)
    {
        const char* msg = gSaveSuccess ? "level saved!" : "save failed!";
        float r = gSaveSuccess ? 0.2f : 1.0f;
        float g = gSaveSuccess ? 1.0f : 0.2f;

        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetColorToAdd(0, 0, 0, 0);
        AEGfxSetColorToMultiply(1, 1, 1, 1);
        AEGfxSetTransparency(1.f);

        AEGfxPrint(gUIFont, msg, 0.1f, 0.85f, 1.0f, r, g, 0.2f, 1.0f);
    }

    // restore
    ApplyWorldCamera();
    AEMtx33Identity(&identity);
    AEGfxSetTransform(identity.m);
}

void GameState_LevelEditor_Free()
{
    if (gInPlayMode)
        SetPlayMode(false);

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