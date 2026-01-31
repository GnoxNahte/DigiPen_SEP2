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
