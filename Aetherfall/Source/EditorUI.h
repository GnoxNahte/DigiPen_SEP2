/*!
@file	EditorUI.h
@author	Santhosh
@brief	Declares the EditorUI system.
        - Handles editor buttons, panels, and on-screen interface elements for the level editor

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
#pragma once
#include "AEEngine.h"

enum class EditorTool
{
    Paint = 0,
    Erase,
    Bind
};

enum class EditorTile
{
    GroundSurface = 0,
    GroundBody,
    GroundBottom,
    Platform,
    Spike,
    PressurePlate,
    Lava,
    Enemy,
    Spawn,
    Vine
};

enum class EditorEnemyPreset
{
    Druid = 0,
    Skeleton = 1,
    Boss = 2
};

struct EditorUIState
{
    float panelW = 220.0f;
    float pad = 12.0f;
    float rowH = 30.0f;
    float gap = 6.0f;

    bool playMode = false;
    bool requestTogglePlay = false;
    bool showGrid = true;
    bool dragPaint = true;

    EditorTool        tool = EditorTool::Paint;
    EditorTile        brush = EditorTile::GroundSurface;
    EditorEnemyPreset enemyPreset = EditorEnemyPreset::Druid;

    // bind mode: currently selected pressure plate id
    int bindSourceTrapId = -1;

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