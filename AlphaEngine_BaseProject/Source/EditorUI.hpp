#pragma once
#include "AEEngine.h"

// tools
enum class EditorTool
{
    Paint = 0,
    Erase
};

// palette
enum class EditorTile
{
    Ground = 0,
    Spike,
    PressurePlate,
    Enemy           // places an enemy spawn marker
};

// which enemy preset to place
enum class EditorEnemyPreset
{
    Druid = 0,
    Skeleton
};

struct EditorUIState
{
    float panelW = 220.0f;
    float pad = 12.0f;
    float rowH = 30.0f;
    float gap = 6.0f;

    bool playMode = false;
    bool showGrid = true;
    bool dragPaint = true;

    EditorTool        tool = EditorTool::Paint;
    EditorTile        brush = EditorTile::Ground;
    EditorEnemyPreset enemyPreset = EditorEnemyPreset::Druid;

    // one-frame actions
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
void EditorUI_SetFont(s8 fontId);

void EditorUI_Draw(EditorUIState& ui, EditorUIIO& io,
    int windowW, int windowH,
    s32 mx, s32 my,
    bool mouseLPressed);