#pragma once
#include "../Player/Player.h"
#include "../Camera.h"
#include "../Environment/MapGrid.h"
#include "../Environment/traps.h"
#include "../enemy/EnemyManager.h"
#include "GSM.h"

class MainMenuScene : public BaseScene
{
public:
    MainMenuScene();
    ~MainMenuScene();
    void Init()   override;
    void Update() override;
    void Render() override;
    void Exit()   override;

private:
    MapGrid      map;
    Player       player;
    Camera       camera;
    TrapManager  trapMgr;
    EnemyManager enemyMgr;

    int mapCols = 40;

    static std::string ExeDir();
};