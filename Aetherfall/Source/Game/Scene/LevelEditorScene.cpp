/*!
@file	LevelEditorScene.cpp
@author	Santhosh
@brief	Defines the LevelEditorScene class.
        - Connects the scene system to the level editor game state functions

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
#include "LevelEditorScene.h"
#include "leveleditor.h"   // gamestate functions
#include "../AudioManager.h"

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
