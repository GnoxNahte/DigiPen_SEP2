// leveleditor.cpp
#include "AEEngine.h"
#include "leveleditor.h"

#include "../Environment/MapGrid.h"
#include "../Environment/MapTile.h"
#include "../Camera.h"
#include "../../Utils/AEExtras.h"

#include "EditorUI.hpp"
#include "LevelIO.h"

// only for Trap::Type enum values (stored as int in TrapDefSimple)
#include "../Environment/traps.h"

#include <vector>
#include <queue>
#include <cmath>
#include <cstdio>

/*========================================================
    configuration
========================================================*/

static constexpr int   GRID_ROWS = 50;
static constexpr int   GRID_COLS = 100;
static constexpr float CAMERA_SCALE = 64.0f;
static constexpr float CAMERA_SPEED = 10.0f;

static const char* LEVEL_PATH = "Assets/level01.lvl";

/*========================================================
    state
========================================================*/

static MapGrid* gMap = nullptr;
static Camera* gCamera = nullptr;

static EditorUIState gUI{};
static EditorUIIO    gUIIO{};

static s8 gUIFont = -1;

// editor data
static std::vector<TrapDefSimple> gTrapDefs;
static AEVec2 gSpawn{ 5.0f, 5.0f };

// debug/status
static char gStatus[128] = "";

// FIX: sample input ONCE per frame in Update, re-use in Draw
static s32  gMouseX = 0, gMouseY = 0;
static bool gMouseLTriggered = false;

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
}

// AEGfxPrint uses normalized coords [-1..1]
static inline float PxToNdcX(float px, float w) { return (px / (w * 0.5f)) - 1.0f; }
static inline float PxToNdcY(float py, float h) { return (py / (h * 0.5f)) - 1.0f; }

static void DebugTextPx(const char* text, float xPx, float yPx)
{
    if (gUIFont < 0 || !text) return;

    const float w = (float)AEGfxGetWindowWidth();
    const float h = (float)AEGfxGetWindowHeight();

    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxPrint(gUIFont, text, PxToNdcX(xPx, w), PxToNdcY(yPx, h), 1.0f, 1, 1, 1, 1);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
}

static MapTile::Type BrushToTile(EditorTile brush)
{
    switch (brush)
    {
    case EditorTile::Ground: return MapTile::Type::GROUND;
    case EditorTile::Empty:  return MapTile::Type::NONE;
    default:                 return MapTile::Type::NONE;
    }
}

static MapTile::Type GetTileType(int x, int y)
{
    const MapTile* t = gMap->GetTile(x, y);
    return t ? t->type : MapTile::Type::NONE;
}

static void RemoveTrapsAtCell(int tx, int ty)
{
    for (size_t i = 0; i < gTrapDefs.size();)
    {
        const int ex = (int)std::floor(gTrapDefs[i].pos.x);
        const int ey = (int)std::floor(gTrapDefs[i].pos.y);

        if (ex == tx && ey == ty)
            gTrapDefs.erase(gTrapDefs.begin() + i);
        else
            ++i;
    }
}

static void PlaceSpikeTrapAtCell(int tx, int ty, float worldX, float worldY)
{
    TrapDefSimple t;
    t.type = (int)Trap::Type::SpikePlate;

    t.pos = gUI.snapToGrid ? AEVec2{ tx + 0.5f, ty + 0.5f } : AEVec2{ worldX, worldY };
    t.size = AEVec2{ 1.0f, 1.0f };

    t.upTime = 1.0f;
    t.downTime = 1.0f;
    t.damageOnHit = 10;
    t.startDisabled = false;

    bool replaced = false;
    for (auto& e : gTrapDefs)
    {
        const int ex = (int)std::floor(e.pos.x);
        const int ey = (int)std::floor(e.pos.y);

        if (ex == tx && ey == ty && e.type == t.type)
        {
            e = t;
            replaced = true;
            break;
        }
    }

    if (!replaced)
        gTrapDefs.push_back(t);
}

static void ClearLevel()
{
    for (int y = 0; y < GRID_ROWS; ++y)
        for (int x = 0; x < GRID_COLS; ++x)
            gMap->SetTile(x, y, MapTile::Type::NONE);

    gTrapDefs.clear();
    gSpawn = AEVec2{ 5.f, 5.f };
    sprintf_s(gStatus, "cleared");
}

static void FloodFillTiles(int sx, int sy, MapTile::Type to)
{
    if (!InBounds(sx, sy)) return;

    const MapTile::Type from = GetTileType(sx, sy);
    if (from == to) return;

    std::queue<std::pair<int, int>> q;
    q.push({ sx, sy });

    while (!q.empty())
    {
        auto [x, y] = q.front();
        q.pop();

        if (!InBounds(x, y)) continue;
        if (GetTileType(x, y) != from) continue;

        gMap->SetTile(x, y, to);

        q.push({ x + 1, y });
        q.push({ x - 1, y });
        q.push({ x, y + 1 });
        q.push({ x, y - 1 });
    }
}

/*========================================================
    editor update
========================================================*/

static void UpdateEditor(float dt)
{
    // play mode freezes editing (ui still works)
    if (gUI.playMode)
        return;

    // camera movement
    if (!gUIIO.mouseCaptured)
    {
        if (AEInputCheckCurr(AEVK_W)) gCamera->position.y += CAMERA_SPEED * dt;
        if (AEInputCheckCurr(AEVK_S)) gCamera->position.y -= CAMERA_SPEED * dt;
        if (AEInputCheckCurr(AEVK_A)) gCamera->position.x -= CAMERA_SPEED * dt;
        if (AEInputCheckCurr(AEVK_D)) gCamera->position.x += CAMERA_SPEED * dt;
    }

    ApplyWorldCamera();

    // block editor interactions if the cursor is over the ui panel
    if (gUIIO.mouseCaptured)
        return;

    // mouse -> world
    AEVec2 world;
    const f32 screenY = (f32)AEGfxGetWindowHeight() - (f32)gMouseY;
    AEExtras::ScreenToWorldPosition(AEVec2{ (f32)gMouseX, screenY }, gCamera->position, world);

    const int tx = (int)std::floor(world.x);
    const int ty = (int)std::floor(world.y);
    if (!InBounds(tx, ty))
        return;

    // drag paint: action is held or triggered
    const bool lmbAction = gUI.dragPaint ? AEInputCheckCurr(AEVK_LBUTTON)
        : AEInputCheckTriggered(AEVK_LBUTTON);

    const bool rmbTrig = AEInputCheckTriggered(AEVK_RBUTTON);

    if (rmbTrig)
    {
        gMap->SetTile(tx, ty, MapTile::Type::NONE);
        RemoveTrapsAtCell(tx, ty);
        return;
    }

    if (!lmbAction)
        return;

    switch (gUI.tool)
    {
    case EditorTool::Paint:
    {
        if (gUI.brush == EditorTile::Spike)
        {
            PlaceSpikeTrapAtCell(tx, ty, world.x, world.y);
        }
        else
        {
            gMap->SetTile(tx, ty, BrushToTile(gUI.brush));
            if (gUI.brush == EditorTile::Empty)
                RemoveTrapsAtCell(tx, ty);
        }
    } break;

    case EditorTool::Erase:
    {
        gMap->SetTile(tx, ty, MapTile::Type::NONE);
        RemoveTrapsAtCell(tx, ty);
    } break;

    case EditorTool::Fill:
    {
        if (gUI.brush == EditorTile::Spike)
        {
            sprintf_s(gStatus, "fill: spike ignored");
        }
        else
        {
            FloodFillTiles(tx, ty, BrushToTile(gUI.brush));
            sprintf_s(gStatus, "fill region");
        }
    } break;

    case EditorTool::Select:
    {
        // select tool: set spawn point
        gSpawn = gUI.snapToGrid ? AEVec2{ tx + 0.5f, ty + 0.5f } : world;
        sprintf_s(gStatus, "spawn set: %.1f %.1f", gSpawn.x, gSpawn.y);
    } break;

    default:
        break;
    }
}

/*========================================================
    GameState lifecycle
========================================================*/

void GameState_LevelEditor_Load()
{
    gUIFont = AEGfxCreateFont("Assets/buggy-font.ttf", 18);
}

void GameState_LevelEditor_Init()
{
    if (!gMap)
        gMap = new MapGrid(GRID_ROWS, GRID_COLS);

    if (!gCamera)
        gCamera = new Camera({ 0.f, 0.f }, { (float)GRID_COLS, (float)GRID_ROWS }, CAMERA_SCALE);

    gCamera->position = { GRID_COLS * 0.5f, GRID_ROWS * 0.5f };
    ApplyWorldCamera();

    EditorUI_Init();
    EditorUI_SetFont(gUIFont);

    gUI = EditorUIState{};
    gUIIO = EditorUIIO{};

    gTrapDefs.clear();
    gSpawn = AEVec2{ 5.f, 5.f };
    sprintf_s(gStatus, "ready");
}

void GameState_LevelEditor_Update()
{
    AEInputUpdate();
    const float dt = (float)AEFrameRateControllerGetFrameTime();

    // sample mouse + click ONCE per frame (FIX for ui buttons)
    AEInputGetCursorPosition(&gMouseX, &gMouseY);
    gMouseLTriggered = AEInputCheckTriggered(AEVK_LBUTTON);

    // IMPORTANT: update capture info for THIS frame so UpdateEditor gating works
   
    const int windowH = (int)AEGfxGetWindowHeight();

    const float mx = (float)gMouseX;
    const float my = (float)(windowH - gMouseY);

    // same condition used by EditorUI_Draw
    gUIIO.mouseCaptured = (mx >= 0.0f && mx <= gUI.panelW && my >= 0.0f && my <= (float)windowH);

    // handle ui actions from PREVIOUS draw call (flags persist until next EditorUI_Draw resets them)
    if (gUI.requestClearMap)
    {
        ClearLevel();
        gUI.requestClearMap = false;
    }

    if (gUI.requestSave)
    {
        LevelData lvl;
        BuildLevelDataFromEditor(*gMap, GRID_ROWS, GRID_COLS, gTrapDefs, gSpawn, lvl);

        if (SaveLevelToFile(LEVEL_PATH, lvl))
            sprintf_s(gStatus, "saved: %s", LEVEL_PATH);
        else
            sprintf_s(gStatus, "save failed");

        gUI.requestSave = false;
    }

    if (gUI.requestLoad)
    {
        LevelData lvl;
        if (LoadLevelFromFile(LEVEL_PATH, lvl) && ApplyLevelDataToEditor(lvl, gMap, gTrapDefs, gSpawn))
            sprintf_s(gStatus, "loaded: %s", LEVEL_PATH);
        else
            sprintf_s(gStatus, "load failed");

        gUI.requestLoad = false;
    }

    if (gUI.requestResetPlayer)
    {
        sprintf_s(gStatus, "reset player (todo)");
        gUI.requestResetPlayer = false;
    }

    UpdateEditor(dt);
}

void GameState_LevelEditor_Draw()
{
    AEGfxSetBackgroundColor(0.15f, 0.15f, 0.15f);

    // ---- draw map (world camera) ----
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    if (gMap && gCamera)
    {
        ApplyWorldCamera();
        gMap->Render(*gCamera);
    }

    // ---- draw ui (screen space) ----
    // NOTE: if ui clicks still don't register, we will sample click here instead.
    EditorUI_Draw(
        gUI, gUIIO,
        (int)AEGfxGetWindowWidth(),
        (int)AEGfxGetWindowHeight(),
        gMouseX, gMouseY,
        gMouseLTriggered
    );

    // ---- debug overlay ----
    if (gUI.showDebug)
    {
        const float w = (float)AEGfxGetWindowWidth();
        const float h = (float)AEGfxGetWindowHeight();

        float x = w - 360.0f;
        float y = h - 40.0f;

        char line[256];

        sprintf_s(line, "mode: %s", gUI.playMode ? "play" : "edit");
        DebugTextPx(line, x, y); y -= 22.0f;

        sprintf_s(line, "tool: %d  brush: %d", (int)gUI.tool, (int)gUI.brush);
        DebugTextPx(line, x, y); y -= 22.0f;

        sprintf_s(line, "spawn: %.1f %.1f", gSpawn.x, gSpawn.y);
        DebugTextPx(line, x, y); y -= 22.0f;

        sprintf_s(line, "traps: %d", (int)gTrapDefs.size());
        DebugTextPx(line, x, y); y -= 22.0f;

        DebugTextPx(gStatus, x, y);
    }
}


void GameState_LevelEditor_Free()
{
    delete gMap; gMap = nullptr;
    delete gCamera; gCamera = nullptr;
    gTrapDefs.clear();
}

void GameState_LevelEditor_Unload()
{
    EditorUI_Shutdown();
}
