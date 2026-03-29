#pragma once
#include "../Player/Player.h"
#include "../Camera.h"
#include "../Environment/MapGrid.h"
#include "../Environment/traps.h"
#include "../enemy/EnemyManager.h"
#include "../enemy/EnemyBoss.h"
#include "../../Game/Rooms/RoomData.h"
#include "../../Game/Rooms/RoomManager.h"
#include "../../Game/Rooms/RoomSystem.h"
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
    EnemyBoss    enemyBoss;
    Camera       camera;

    RoomManager  roomMgr;
    RoomSystem   roomSystem;

    int mapCols = 100;
    int mapRows = 100;
    int uiFont = -1;

    AEGfxTexture* vineTexture = nullptr;
    AEGfxVertexList* vineMesh = nullptr;
    std::vector<AEVec2> vinePositions;

    AEGfxVertexList* fadeMesh = nullptr;
    bool  isFadingToGame = false;
    float fadeAlpha = 0.0f;
    float fadeSpeed = 3.1f;

    struct RunRecord
    {
        int levelsCleared = 0;
        double timeSeconds = 0.0;
        bool valid = false;
    };

    RunRecord personalBest;
    RunRecord latestRun;

    void LoadRunRecords();
    std::string FormatTime(double seconds) const;
    void RenderLeaderboard(float worldX, float worldY) const;
};