/*!
@file	MainMenuScene.h
@author	Santhosh,Ethan,Wei Xiang,Lim Kang Pin,Cheng
@brief	Declares the MainMenuScene class.
        - Manages the main menu scene lifecycle, including initialization, update, render, and exit

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
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
#include "../Credits.h"
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
    int uiFontLarge = -1;
    int quitConfirmFont = -1;
    int quitConfirmFontLarge = -1;

    AEGfxTexture* vineTexture = nullptr;
    AEGfxVertexList* vineMesh = nullptr;
    std::vector<AEVec2> vinePositions;

    AEGfxVertexList* fadeMesh = nullptr;
    bool  isFadingToGame = false;
    float fadeAlpha = 0.0f;
    float fadeSpeed = 3.1f;

    float fadeSettingsAlpha = 0.f;

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

    struct TriggerZone
    {
        float minX = 0.f;
        float maxX = 0.f;
        float minY = 0.f;
        float maxY = 0.f;
    };

    bool draggingMasterSlider = false;
    bool draggingBgmSlider = false;
    bool draggingSfxSlider = false;

    struct UIRect
    {
        AEVec2 pos;
        AEVec2 size;
    };

    TriggerZone settingsTrigger;
    TriggerZone startGameTrigger;
    TriggerZone creditsTrigger;

    Credits credits;

    bool IsPlayerInsideTrigger(const TriggerZone& t) const;
    bool IsMenuOpen() const;
    void UpdateMenuInput();
    void RenderMenuOverlay();

    bool isQuitConfirmOpen = false;
    bool quitHoverYesLastFrame = false;
    bool quitHoverNoLastFrame = false;

    void UpdateQuitConfirmInput();
    void RenderQuitConfirmOverlay();

    void DrawDimBackground(float alpha);
    void DrawSolidPanel(const UIRect& r, float alpha);
    void DrawTextPx(s8 font, const std::string& text, float px, float py, float scale,
        float r, float g, float b, float a);
    bool IsMouseOver(const UIRect& r) const;
    bool IsClicked(const UIRect& r) const;

    bool inCredits = false;

    void OnCreditsEnter();
    void OnCreditsExit();
};