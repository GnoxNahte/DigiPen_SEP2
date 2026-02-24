#include "LevelEditorScene.h"
#include "leveleditor.h"   // gamestate functions

LevelEditorScene::LevelEditorScene()
{
    GameState_LevelEditor_Load();
}

LevelEditorScene::~LevelEditorScene()
{
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
