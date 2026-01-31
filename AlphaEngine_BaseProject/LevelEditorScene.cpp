#include "LevelEditorScene.h"
#include "leveleditor.hpp"   // your gamestate functions

LevelEditorScene::LevelEditorScene()
{
    // your project already uses: load() moved to constructor
    GameState_LevelEditor_Load();
}

LevelEditorScene::~LevelEditorScene()
{
    // your project already uses: unload() moved to destructor
    GameState_LevelEditor_Unload();
}

void LevelEditorScene::Init()
{
    GameState_LevelEditor_Init();
}

void LevelEditorScene::Update()
{
    GameState_LevelEditor_Update();
}

void LevelEditorScene::Render()
{
    GameState_LevelEditor_Draw();
}

void LevelEditorScene::Exit()
{
    GameState_LevelEditor_Free();
}
