/*!
@file	LevelEditorScene.h
@author	Santhosh
@brief	Declares the LevelEditorScene class.
        - Manages the level editor scene lifecycle, including init, update, render, and exit

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
#pragma once
#include "GSM.h"          // base scene interface

class LevelEditorScene : public BaseScene
{
public:
    LevelEditorScene();
    ~LevelEditorScene() override;

    void Init() override;
    void Update() override;
    void Render() override;
    void Exit() override;
};
