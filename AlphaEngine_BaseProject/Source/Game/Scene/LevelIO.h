// leveleditor.cpp
#include "AEEngine.h"            // keep engine first (prevents prototype weirdness)
#include "leveleditor.h"

#include "../Environment/MapGrid.h"
#include "../Environment/MapTile.h"
#include "../Camera.h"
#include "../../Utils/AEExtras.h"

#include "EditorUI.hpp"
#include "LevelIO.h"             // NEW: save/load + trap defs

#include <vector>
#include <cmath>                 // floorf

/*========================================================
    configuration
========================================================*/

// IMPORTANT: MapGrid in your codebase is MapGrid(rows, cols)
static constexpr int GRID_ROWS = 50;
static constexpr int GRID_COLS = 100;

static constexpr float CAMERA_SCALE = 64.0f;   // pixels per tile
static constexpr float CAMERA_SPEED = 10.0f;   // tiles per second

// where to save
static const char* LEVEL_PATH = "Assets/level01.lvl";

/*========================================================
    editor state
========================================================*/

static MapGrid* gMap = nullptr;
static Camera* gCamera = nullptr;

// ui
static EditorUIState gUI{};
static EditorUIIO    gUIIO{};
static s8            gUIFont = -1;

// edit/play toggle (your editor already uses this)
static int gPlayMode = 0; // 0 = edit, 1 = play

// backend extras
static std::vector<TrapDef> gTrapDefs; // traps placed in editor
static AEVec2 gSpawn{ 5.0f, 5.0f };    // optional spawn point

/*========================================================
    helpers
========================================================*/

static void ApplyCamera()
{
    if (!gCamera) return;

    AEGfxSetCamPosition(
        gCamera->position.x * Camera::scale,
        gCamera->position.y * Camera::scale
    );
}

// ui brush -> tile type (ONLY for solid terrain)
static MapTile::Type BrushToTile(EditorTile brush)
{
    switch (brush)
    {
    case EditorTile::Ground: return MapTile::Type::GROUND;
    case EditorTile::Empty:  return MapTile::Type::NONE;
    default:                 return MapTile::Type::NONE;
    }
}

static void RemoveTrapsAtCell(int tx, int ty)
{
    for (size_t i = 0; i < gTrapDefs.size(); )
    {
        const int ex = (int)std::floor(gTrapDefs[i].box.position.x);
        const int ey = (int)std::floor(gTrapDefs[i].box.position.y);

        if (ex == tx && ey == ty)
            gTrapDefs.erase(gTrapDefs.begin() + i);
        else
            ++i;
    }
}

static void PlaceSpikeTrapAtCell(int tx, int ty)
{
    TrapDef t;
    t.type = Trap::Type::SpikePlate;

    // box centered inside tile
    t.box.position = AEVec2{ tx + 0.5f, ty + 0.5f };
    t.box.size = AEVec2{ 1.0f, 1.0f };

    // default params
    t.upTime = 1.0f;
    t.downTime = 1.0f;
    t.damageOnHit = 10;
    t.startDisabled = false;

    // replace existing spike at same cell (optional)
    bool replaced = false;
    for (auto& e : gTrapDefs)
    {
        const int ex = (int)std::floor(e.box.position.x);
        const int ey = (int)std::floor(e.box.position.y);

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

/*========================================================
    editor update
========================================================*/

static void UpdateEditor(float dt)
{
    // camera movement (disabled while ui capturing mouse)
    if (!gUIIO.mouseCaptured)
    {
        if (AEInputCheckCurr(AEVK_W)) gCamera->position.y += CAMERA_SPEED * dt;
        if (AEInputCheckCurr(AEVK_S)) gCamera->position.y -= CAMERA_SPEED * dt;
        if (AEInputCheckCurr(AEVK_A)) gCamera->position.x -= CAMERA_SPEED * dt;
        if (AEInputCheckCurr(AEVK_D)) gCamera->position.x += CAMERA_SPEED * dt;
    }

    ApplyCamera();

    // if ui is using the mouse, do not paint/place
    if (gUIIO.mouseCaptured)
        return;

    // mouse -> world -> tile
    s32 mx, my;
    AEInputGetCursorPosition(&mx, &my);

    AEVec2 world;
    const f32 screenY = (f32)AEGfxGetWindowHeight() - (f32)my;
    AEExtras::ScreenToWorldPosition(AEVec2{ (f32)mx, screenY }, gCamera->position, world);

    const int tx = (int)std::floor(world.x);
    const int ty = (int)std::floor(world.y);

    // bounds check (optional safety)
    if (tx < 0 || tx >= GRID_COLS || ty < 0 || ty >= GRID_ROWS)
        return;

    const bool lmbHeld = AEInputCheckCurr(AEVK_LBUTTON);
    const bool rmbTrig = AEInputCheckTriggered(AEVK_RBUTTON);

    // right click: erase tile + remove traps at cell
    if (rmbTrig)
    {
        gMap->SetTile(tx, ty, MapTile::Type::NONE);
        RemoveTrapsAtCell(tx, ty);
        return;
    }

    // left hold: tool action
    if (!lmbHeld)
        return;

    switch (gUI.tool)
    {
    case EditorTool::Paint:
    {
        if (gUI.brush == EditorTile::Spike)
        {
            // spike is NOT a tile in your MapTile::Type, treat as entity
            PlaceSpikeTrapAtCell(tx, ty);
        }
        else
        {
            gMap->SetTile(tx, ty, BrushToTile(gUI.brush));
        }
    } break;

    case EditorTool::Erase:
    {
        gMap->SetTile(tx, ty, MapTile::Type::NONE);
        RemoveTrapsAtCell(tx, ty);
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
    // IMPORTANT: compile-safe even if some TU sees CreateFont as s32
    const auto fontId = AEGfxCreateFont("Assets/buggy-font.ttf", 18);
    gUIFont = static_cast<s8>(fontId);
}

void GameState_LevelEditor_Init()
{
    // create map
    if (!gMap)
        gMap = new MapGrid(GRID_ROWS, GRID_COLS);

    // create camera (tile space bounds)
    if (!gCamera)
        gCamera = new Camera(
            { 0.f, 0.f },
            { (float)GRID_COLS, (float)GRID_ROWS },
            CAMERA_SCALE
        );

    gCamera->position = { GRID_COLS * 0.5f, GRID_ROWS * 0.5f };
    ApplyCamera();

    // init ui
    EditorUI_Init();
    EditorUI_SetFont(gUIFont);

    gUI = EditorUIState{};
    gUIIO = EditorUIIO{};

    // reset backend state
    gTrapDefs.clear();
    gSpawn = AEVec2{ 5.f, 5.f };
}

void GameState_LevelEditor_Update()
{
    AEInputUpdate();
    const float dt = (float)AEFrameRateControllerGetFrameTime();

    // draw/update ui FIRST so mouseCaptured is valid for editor logic
    s32 mx, my;
    AEInputGetCursorPosition(&mx, &my);
    EditorUI_Draw(
        gUI, gUIIO,
        (int)AEGfxGetWindowWidth(),
        (int)AEGfxGetWindowHeight(),
        mx, my,
        AEInputCheckTriggered(AEVK_LBUTTON)
    );

    // handle ui actions (save/load/clear)
    if (gUI.requestSave)
    {
        LevelData lvl;
        BuildLevelDataFromEditor(*gMap, GRID_ROWS, GRID_COLS, gTrapDefs, gSpawn, lvl);
        SaveLevelToFile(LEVEL_PATH, lvl);
    }

    if (gUI.requestLoad)
    {
        LevelData lvl;
        if (LoadLevelFromFile(LEVEL_PATH, lvl))
            ApplyLevelDataToEditor(lvl, gMap, gTrapDefs, gSpawn);
    }

    if (gUI.requestClearMap)
    {
        for (int y = 0; y < GRID_ROWS; ++y)
            for (int x = 0; x < GRID_COLS; ++x)
                gMap->SetTile(x, y, MapTile::Type::NONE);

        gTrapDefs.clear();
    }

    // toggle play/edit (optional)
    if (AEInputCheckTriggered(AEVK_TAB))
        gPlayMode = !gPlayMode;

    if (!gPlayMode)
        UpdateEditor(dt);

    // NOTE: play mode hook (later):
    // if (gPlayMode) spawn real traps into TrapManager from gTrapDefs
}

void GameState_LevelEditor_Draw()
{
    AEGfxSetBackgroundColor(0.15f, 0.15f, 0.15f);
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);

    // draw map
    if (gMap && gCamera)
        gMap->Render(*gCamera);

    // TODO (later): draw trap previews in editor
    // e.g. draw colored boxes or icons at gTrapDefs positions

    // ui overlay
    s32 mx, my;
    AEInputGetCursorPosition(&mx, &my);
    EditorUI_Draw(
        gUI, gUIIO,
        (int)AEGfxGetWindowWidth(),
        (int)AEGfxGetWindowHeight(),
        mx, my,
        AEInputCheckTriggered(AEVK_LBUTTON)
    );
}

void GameState_LevelEditor_Free()
{
    delete gMap;
    gMap = nullptr;

    delete gCamera;
    gCamera = nullptr;

    gTrapDefs.clear();
}

void GameState_LevelEditor_Unload()
{
    EditorUI_Shutdown();
}
