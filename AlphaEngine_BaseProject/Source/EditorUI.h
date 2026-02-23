#pragma once
#include "AEEngine.h"

// tools
enum class EditorTool
{
    Paint = 0,
    Erase,
    Fill,
    Select
};

// palette tiles
enum class EditorTile
{
    Empty = 0,
    Ground,
    Spike
};

struct EditorUIState
{
    // panel layout
    float panelW = 260.0f;
    float pad = 12.0f;
    float rowH = 34.0f;
    float gap = 8.0f;

    // state controlled by ui
    bool playMode = false;
    bool showGrid = true;
    bool snapToGrid = true;
    bool dragPaint = true;
    bool showDebug = true;

    int brushSize = 1;
    EditorTool tool = EditorTool::Paint;
    EditorTile brush = EditorTile::Ground;

    // one-frame actions
    bool requestResetPlayer = false;
    bool requestClearMap = false;
    bool requestSave = false;
    bool requestLoad = false;
};

struct EditorUIIO
{
    bool mouseCaptured = false;
    bool keyboardCaptured = false;
};

void EditorUI_Init();
void EditorUI_Shutdown();

// set font used by AEGfxPrint
void EditorUI_SetFont(s8 fontId);

// draw ui (mx,my are from AEInputGetCursorPosition, origin top-left)
void EditorUI_Draw(EditorUIState& ui, EditorUIIO& io,
    int windowW, int windowH,
    s32 mx, s32 my,
    bool mouseLPressed);
