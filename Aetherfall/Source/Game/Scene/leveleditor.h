/*!
@file	leveleditor.h
@author	Santhosh
@brief	Declares the LevelEditor system.
        - Handles tile placement, object placement, selection, and editor-side level editing logic

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
#pragma once
#pragma once

// lifecycle functions called by LevelEditorScene
void GameState_LevelEditor_Load();
void GameState_LevelEditor_Init();
void GameState_LevelEditor_Update();
void GameState_LevelEditor_Draw();
void GameState_LevelEditor_Free();
void GameState_LevelEditor_Unload();
