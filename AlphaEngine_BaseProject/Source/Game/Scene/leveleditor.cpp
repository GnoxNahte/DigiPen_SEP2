#include "leveleditor.h"

#include "AEEngine.h"
#include "EditorUI.hpp"

/*========================================================
    editor state data
========================================================*/
static s8 gEditorFont = -1;

static EditorUIState gUI{};
static EditorUIIO gUIIO{};

/*========================================================
    lifecycle
========================================================*/
void GameState_LevelEditor_Load()
{
    // load font (safe even if called once)
    gEditorFont = AEGfxCreateFont("Assets/buggy-font.ttf", 18);
}

void GameState_LevelEditor_Init()
{
    EditorUI_Init();

    if (gEditorFont >= 0)
        EditorUI_SetFont(gEditorFont);

    // reset ui state each time you enter the editor
    gUI = EditorUIState{};
    gUIIO = EditorUIIO{};
}

void GameState_LevelEditor_Update()
{
    AEInputUpdate();

    // later: handle world updates here (camera, paint tiles, play mode, etc.)
    // use gUI.playMode and gUIIO.mouseCaptured to gate inputs
}

void GameState_LevelEditor_Draw()
{
    // clear background
    AEGfxSetBackgroundColor(0.12f, 0.12f, 0.12f);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);

    // later: draw world here (map, player, etc.)

    // draw editor ui panels LAST (overlay)
    s32 mx, my;
    AEInputGetCursorPosition(&mx, &my);
    bool mousePressed = AEInputCheckTriggered(AEVK_LBUTTON);
    EditorUI_Draw(gUI, gUIIO, 1600, 900, mx, my, mousePressed);

    // debug text (top-left corner)
    if (gEditorFont >= 0)
    {
        AEGfxPrint(
            gEditorFont,
            "LEVEL EDITOR MODE",
            20.0f, 880.0f,
            1.0f,
            0.9f, 0.9f, 0.9f, 1.0f
        );
    }
}

void GameState_LevelEditor_Free()
{
    // nothing heavy here
}

void GameState_LevelEditor_Unload()
{
    EditorUI_Shutdown();
}
