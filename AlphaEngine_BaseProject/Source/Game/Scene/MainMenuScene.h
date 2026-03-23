#pragma once
#include "../Player/Player.h"
#include "../Camera.h"
#include "../Environment/MapGrid.h"
#include "../Environment/traps.h"
#include "../enemy/EnemyManager.h"
#include "../../Game/Rooms/RoomData.h"
#include "GSM.h"
#include <string>
#include <vector>

class MainMenuScene : public BaseScene
{
public:
    MainMenuScene();
    ~MainMenuScene();

    void Init() override;
    void Update() override;
    void Render() override;
    void Exit() override;

private:
    static std::string ExeDir();

    MapGrid      map;
    Player       player;
    TrapManager  trapMgr;
    EnemyManager enemyMgr;
    Camera       camera;

    int mapCols = 100;
    int mapRows = 100;
    int uiFont = -1;

    // vine decoration
    AEGfxTexture* vineTexture = nullptr;
    AEGfxVertexList* vineMesh = nullptr;
    std::vector<AEVec2> vinePositions;


};