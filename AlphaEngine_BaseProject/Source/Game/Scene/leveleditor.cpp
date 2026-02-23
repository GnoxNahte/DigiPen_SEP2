// leveleditor.cpp
#include "AEEngine.h"
#include "leveleditor.h"

#include "../Environment/MapGrid.h"
#include "../Environment/MapTile.h"
#include "../Camera.h"
#include "../../Utils/AEExtras.h"

#include "EditorUI.hpp"
#include "LevelIO.h"
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
static bool gPlayMode = false;
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

static MapTile::Type BrushToTile(EditorTile brush)
{
    switch (brush)
    {
    case EditorTile::Ground: return MapTile::Type::GROUND;
    case EditorTile::Empty:  return MapTile::Type::NONE;
    default:                 return MapTile::Type::NONE;
    }
}

//static MapTile::Type GetTileType(int x, int y)
//{
  //  if (!gMap) return MapTile::Type::NONE;

  //  const MapTile* t = gMap->GetTile(x, y);
 //   return t ? t->type : MapTile::Type::NONE;
//}

static void RemoveTrapsAtCell(int tx, int ty)
{
    for (size_t i = 0; i < gTrapDefs.size();)
    {
        const int ex = (int)std::floor(gTrapDefs[i].pos.x);
        const int ey = (int)std::floor(gTrapDefs[i].pos.y);

        if (ex == tx && ey == ty)
            gTrapDefs.erase(gTrapDefs.begin() + (ptrdiff_t)i);
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

    // replace same trap at same cell if exists
    for (auto& e : gTrapDefs)
    {
        const int ex = (int)std::floor(e.pos.x);
        const int ey = (int)std::floor(e.pos.y);

        if (ex == tx && ey == ty && e.type == t.type)
        {
            e = t;
            return;
        }
    }

    gTrapDefs.push_back(t);
}

/*========================================================
    editor update
========================================================*/

static void UpdateEditor(float dt)
{
    // IMPORTANT: guards fix C6011 and also prevents real crashes
    if (!gMap || !gCamera)
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

    // block editor interactions if cursor is over ui
    if (gUIIO.mouseCaptured)
        return;

    // mouse -> world (your old conversion)
    s32 mx, my;
    AEInputGetCursorPosition(&mx, &my);

    AEVec2 world;
    const f32 screenY = (f32)AEGfxGetWindowHeight() - (f32)my;
    AEExtras::ScreenToWorldPosition(AEVec2{ (f32)mx, screenY }, gCamera->position, world);

    const int tx = (int)std::floor(world.x);
    const int ty = (int)std::floor(world.y);
    if (!InBounds(tx, ty))
        return;

    const bool lmbHeld = gUI.dragPaint ? AEInputCheckCurr(AEVK_LBUTTON)
        : AEInputCheckTriggered(AEVK_LBUTTON);
    const bool rmbTrig = AEInputCheckTriggered(AEVK_RBUTTON);

    if (rmbTrig)
    {
        gMap->SetTile(tx, ty, MapTile::Type::NONE);
        RemoveTrapsAtCell(tx, ty);
        return;
    }

    if (!lmbHeld)
        return;

    switch (gUI.tool)
    {
    case EditorTool::Paint:
        if (gUI.brush == EditorTile::Spike)
            PlaceSpikeTrapAtCell(tx, ty, world.x, world.y);
        else
            gMap->SetTile(tx, ty, BrushToTile(gUI.brush));
        break;

    case EditorTool::Erase:
        gMap->SetTile(tx, ty, MapTile::Type::NONE);
        RemoveTrapsAtCell(tx, ty);
        break;

    default:
        break;
    }
}

/*========================================================
    GameState lifecycle
========================================================*/

void GameState_LevelEditor_Load()
{
    const auto fontId = AEGfxCreateFont("Assets/buggy-font.ttf", 18);
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

    if (gCamera)
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
    // guard fixes analyzer warning if Update is ever called before Init
    if (!gMap || !gCamera)
        return;

    AEInputUpdate();
    const float dt = (float)AEFrameRateControllerGetFrameTime();

    // ui input sampling
    s32 mx, my;
    AEInputGetCursorPosition(&mx, &my);

    EditorUI_Draw(
        gUI, gUIIO,
        (int)AEGfxGetWindowWidth(),
        (int)AEGfxGetWindowHeight(),
        mx, my,
        AEInputCheckTriggered(AEVK_LBUTTON)
    );

    // save
    if (gUI.requestSave)
    {
        LevelData lvl;
        BuildLevelDataFromEditor(*gMap, GRID_ROWS, GRID_COLS, gTrapDefs, gSpawn, lvl);

        if (SaveLevelToFile(LEVEL_PATH, lvl))
            sprintf_s(gStatus, "saved");
        else
            sprintf_s(gStatus, "save failed");

        gUI.requestSave = false;
    }

    // load
    if (gUI.requestLoad)
    {
        LevelData lvl;
        if (LoadLevelFromFile(LEVEL_PATH, lvl) &&
            ApplyLevelDataToEditor(lvl, gMap, gTrapDefs, gSpawn))
            sprintf_s(gStatus, "loaded");
        else
            sprintf_s(gStatus, "load failed");

        gUI.requestLoad = false;
    }

    // clear
    if (gUI.requestClearMap)
    {
        for (int y = 0; y < GRID_ROWS; ++y)
            for (int x = 0; x < GRID_COLS; ++x)
                gMap->SetTile(x, y, MapTile::Type::NONE);

        gTrapDefs.clear();
        sprintf_s(gStatus, "cleared");

        gUI.requestClearMap = false;
    }

    // toggle edit/play
    if (AEInputCheckTriggered(AEVK_TAB))
        gPlayMode = !gPlayMode;

    if (!gPlayMode)
        UpdateEditor(dt);
}

void GameState_LevelEditor_Draw()
{
    // guard fixes analyzer warning if Draw is ever called before Init
    if (!gMap || !gCamera)
        return;

    AEGfxSetBackgroundColor(0.15f, 0.15f, 0.15f);
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);

    ApplyWorldCamera();
    gMap->Render(*gCamera);

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
    delete gMap; gMap = nullptr;
    delete gCamera; gCamera = nullptr;
    gTrapDefs.clear();
}

void GameState_LevelEditor_Unload()
{
    EditorUI_Shutdown();
}