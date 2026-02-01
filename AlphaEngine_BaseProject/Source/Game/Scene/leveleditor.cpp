#include "leveleditor.h"

#include "../Environment/MapGrid.h"
#include "../Environment/MapTile.h"
#include "../../Utils/MeshGenerator.h"
#include "../Camera.h"
#include "../../Utils/AEExtras.h"
#include "EditorUI.hpp"

#include <math.h>
#include <vector>
/*========================================================
	editor + world configuration
========================================================*/

// grid size in tiles
static constexpr int GRID_COLS = 100;
static constexpr int GRID_ROWS = 50;

// camera scale (pixels per tile)
static constexpr float CAMERA_SCALE = 64.0f;

// camera movement speed (tiles / second)
static constexpr float CAMERA_SPEED = 10.0f;

/*========================================================
	editor state
========================================================*/

static MapGrid* gMap = nullptr;
static Camera* gCamera = nullptr;

// editor ui
static EditorUIState gUI{};
static EditorUIIO    gUIIO{};
static s8            gUIFont = -1;

// editor mode
static int gPlayMode = 0; // 0 = edit, 1 = play

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

// map EditorUI brush → MapTile type
static MapTile::Type BrushToTile(EditorTile brush)
{
	switch (brush)
	{
	case EditorTile::Ground: return MapTile::Type::GROUND;
	case EditorTile::Empty:  return MapTile::Type::NONE;
	default:                 return MapTile::Type::NONE;
	}
}

/*========================================================
	editor update (tile placement happens here)
========================================================*/

static void UpdateEditor(float dt)
{
	// ------------------------------
	// camera movement (WASD)
	// disabled while UI is capturing mouse
	// ------------------------------
	if (!gUIIO.mouseCaptured)
	{
		if (AEInputCheckCurr(AEVK_W)) gCamera->position.y += CAMERA_SPEED * dt;
		if (AEInputCheckCurr(AEVK_S)) gCamera->position.y -= CAMERA_SPEED * dt;
		if (AEInputCheckCurr(AEVK_A)) gCamera->position.x -= CAMERA_SPEED * dt;
		if (AEInputCheckCurr(AEVK_D)) gCamera->position.x += CAMERA_SPEED * dt;
	}

	ApplyCamera();

	// if UI is using the mouse, do not paint
	if (gUIIO.mouseCaptured)
		return;

	// ------------------------------
	// mouse → world conversion
	// IMPORTANT: must match MapGrid::Render()
	// ------------------------------
	s32 mx, my;
	AEInputGetCursorPosition(&mx, &my);

	AEVec2 world;
	const f32 screenY = (f32)AEGfxGetWindowHeight() - (f32)my;
	AEExtras::ScreenToWorldPosition(AEVec2{ (f32)mx, screenY }, gCamera->position, world);
	
	const int tx = (int)floorf(world.x);
	const int ty = (int)floorf(world.y);

	// ------------------------------
	// mouse input
	// ------------------------------
	const bool lmb = AEInputCheckCurr(AEVK_LBUTTON);
	const bool rmb = AEInputCheckTriggered(AEVK_RBUTTON);

	// right click = erase
	if (rmb)
	{
		gMap->SetTile(tx, ty, MapTile::Type::NONE);
		return;
	}

	// left click = tool action
	if (!lmb)
		return;

	switch (gUI.tool)
	{
	case EditorTool::Paint:
		gMap->SetTile(tx, ty, BrushToTile(gUI.brush));
		break;

	case EditorTool::Erase:
		gMap->SetTile(tx, ty, MapTile::Type::NONE);
		break;

		// Fill / Select can be added later
	default:
		break;
	}
}

/*========================================================
	GameState lifecycle
========================================================*/

void GameState_LevelEditor_Load()
{
	// load font for editor UI
	gUIFont = AEGfxCreateFont("Assets/buggy-font.ttf", 18);
}

void GameState_LevelEditor_Init()
{
	// create map
	if (!gMap)
		gMap = new MapGrid(GRID_COLS, GRID_ROWS);

	// create camera (tile space)
	if (!gCamera)
		gCamera = new Camera(
			{ 0.f, 0.f },
			{ (float)GRID_COLS, (float)GRID_ROWS },
			CAMERA_SCALE
		);

	gCamera->position = { GRID_COLS * 0.5f, GRID_ROWS * 0.5f };
	ApplyCamera();

	// init editor ui
	EditorUI_Init();
	if (gUIFont >= 0)
		EditorUI_SetFont(gUIFont);

	gUI = EditorUIState{};
	gUIIO = EditorUIIO{};
}

void GameState_LevelEditor_Update()
{
	AEInputUpdate();
	const float dt = (float)AEFrameRateControllerGetFrameTime();

	// IMPORTANT:
	// update EditorUI here so mouseCaptured is valid THIS frame
	s32 mx, my;
	AEInputGetCursorPosition(&mx, &my);
	EditorUI_Draw(
		gUI, gUIIO,
		(int)AEGfxGetWindowWidth(),
		(int)AEGfxGetWindowHeight(),
		mx, my,
		AEInputCheckTriggered(AEVK_LBUTTON)
	);

	// toggle play/edit
	if (AEInputCheckTriggered(AEVK_TAB))
		gPlayMode = !gPlayMode;

	if (!gPlayMode)
		UpdateEditor(dt);
}

void GameState_LevelEditor_Draw()
{
	AEGfxSetBackgroundColor(0.15f, 0.15f, 0.15f);
	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);

	// draw map
	if (gMap && gCamera)
		gMap->Render(*gCamera);

	// draw UI last (overlay)
	s32 mx, my;
	AEInputGetCursorPosition(&mx, &my);
	EditorUI_Draw(
		gUI, gUIIO,
		(int)AEGfxGetWindowWidth(),
		(int)AEGfxGetWindowHeight(),
		mx, my,
		AEInputCheckTriggered(AEVK_LBUTTON)
	);

	// debug text
	if (gUIFont >= 0)
	{
		AEGfxPrint(
			gUIFont,
			"LEVEL EDITOR",
			20.f, 880.f,
			1.f,
			1.f, 1.f, 1.f, 1.f
		);
	}
}

void GameState_LevelEditor_Free()
{
	delete gMap;
	gMap = nullptr;

	delete gCamera;
	gCamera = nullptr;
}

void GameState_LevelEditor_Unload()
{
	EditorUI_Shutdown();
}