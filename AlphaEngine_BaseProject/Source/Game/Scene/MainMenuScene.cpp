#include "MainMenuScene.h"
#include "AEEngine.h"
#include "LevelIO.h"
#include "GSM.h"
#include "../../Game/Rooms/RoomBuilder.h"
#include "../Environment/MapTile.h"
#include "../enemy/Enemy.h"
#include "../Time.h"
#include "../AudioManager.h"
#include "../UI.h"
#include "../../Editor/Editor.h"
#include "../../Utils/AEExtras.h"
#include "../../Utils/QuickGraphics.h"
#include "../Background.h"
#include <Windows.h>
#include <new>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>

std::string MainMenuScene::ExeDir()
{
    char buf[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    std::string s(buf);
    size_t slash = s.find_last_of("\\/");
    return (slash != std::string::npos) ? s.substr(0, slash + 1) : ".\\";
}

MainMenuScene::MainMenuScene()
    : map(20, 40)
    , player(&map, &enemyMgr)
    , enemyBoss()
    , camera({ 1.f, 1.f }, { 50.f, 50.f }, 64.f)
    , roomSystem(map, player, camera, trapMgr, enemyMgr, enemyBoss, roomMgr)
{
}

MainMenuScene::~MainMenuScene()
{
}

bool MainMenuScene::IsPlayerInsideTrigger(const TriggerZone& t) const
{
    AEVec2 p = player.GetPosition();
    return (p.x >= t.minX && p.x <= t.maxX &&
        p.y >= t.minY && p.y <= t.maxY);
}

bool MainMenuScene::IsMenuOpen() const
{
    return fadeSettingsAlpha > 0.f;
}

bool MainMenuScene::IsMouseOver(const UIRect& r) const
{
    return Button::CheckMouseInRectButton(r.pos, r.size);
}

bool MainMenuScene::IsClicked(const UIRect& r) const
{
    return IsMouseOver(r) && AEInputCheckTriggered(AEVK_LBUTTON);
}

void MainMenuScene::DrawTextPx(s8 font, const std::string& text, float px, float py, float scale,
    float r, float g, float b, float a)
{
    float w = (float)AEGfxGetWindowWidth();
    float h = (float)AEGfxGetWindowHeight();

    float xNdc = (px / w) * 2.0f - 1.0f;
    float yNdc = 1.0f - (py / h) * 2.0f;

    AEGfxPrint(font, text.c_str(), xNdc, yNdc, scale, r, g, b, a);
}

static AEVec2 MM_ScreenToEngine(float px, float py)
{
    float w = (float)AEGfxGetWindowWidth();
    float h = (float)AEGfxGetWindowHeight();
    return AEVec2{ px - w * 0.5f, (h * 0.5f) - py };
}

void MainMenuScene::DrawDimBackground(float alpha)
{
    if (alpha <= 0.0f || !fadeMesh) return;

    AEMtx33 scale, rotate, translate, transform;

    AEMtx33Scale(&scale,
        (float)AEGfxGetWindowWidth() * 2.0f,
        (float)AEGfxGetWindowHeight() * 2.0f);

    AEMtx33Rot(&rotate, 0.0f);

    AEMtx33Trans(&translate,
        Camera::position.x * Camera::scale,
        Camera::position.y * Camera::scale);

    AEMtx33Concat(&transform, &rotate, &scale);
    AEMtx33Concat(&transform, &translate, &transform);

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetColorToMultiply(0.f, 0.f, 0.f, 1.f);
    AEGfxSetColorToAdd(0.f, 0.f, 0.f, 0.f);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(alpha);

    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(fadeMesh, AE_GFX_MDM_TRIANGLES);
}

void MainMenuScene::DrawSolidPanel(const UIRect& r, float alpha)
{
    if (!fadeMesh) return;

    AEMtx33 scale, rot, trans, transform;
    AEMtx33Scale(&scale, r.size.x, r.size.y);
    AEMtx33Rot(&rot, 0.0f);

    AEVec2 eng = MM_ScreenToEngine(r.pos.x, r.pos.y);
    AEMtx33Trans(&trans,
        eng.x + Camera::position.x * Camera::scale,
        eng.y + Camera::position.y * Camera::scale);

    AEMtx33Concat(&transform, &rot, &scale);
    AEMtx33Concat(&transform, &trans, &transform);

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetColorToMultiply(0.f, 0.f, 0.f, 0.f);
    AEGfxSetColorToAdd(0.f, 0.f, 0.f, alpha);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(alpha);

    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(fadeMesh, AE_GFX_MDM_TRIANGLES);
}

void MainMenuScene::UpdateMenuInput()
{
    auto Clamp01_UI = [](float v) -> float {
        if (v < 0.0f) return 0.0f;
        if (v > 1.0f) return 1.0f;
        return v;
        };

    auto PointInRectPx = [](float mx, float my, const UIRect& r) -> bool {
        const float left = r.pos.x - r.size.x * 0.5f;
        const float right = r.pos.x + r.size.x * 0.5f;
        const float top = r.pos.y - r.size.y * 0.5f;
        const float bottom = r.pos.y + r.size.y * 0.5f;
        return (mx >= left && mx <= right && my >= top && my <= bottom);
        };

    auto SliderValueFromMouse = [&](float mouseX, float leftX, float width) -> float {
        if (width <= 0.0f) return 0.0f;
        return Clamp01_UI((mouseX - leftX) / width);
        };

    const float sliderLeft = 860.0f;
    const float sliderWidth = 330.0f;
    const float knobSize = 28.0f;
    const float hitboxHeight = 36.0f;
    const float masterY = 240.f;
    const float bgmY = 350.0f;
    const float sfxY = 460.0f;

    s32 mx = 0, my = 0;
    AEInputGetCursorPosition(&mx, &my);

    const bool mousePressed = AEInputCheckTriggered(AEVK_LBUTTON);
    const bool mouseHeld = AEInputCheckCurr(AEVK_LBUTTON);

    auto MakeTrackRect = [&](float y) -> UIRect {
        return UIRect{ { sliderLeft + sliderWidth * 0.5f, y }, { sliderWidth, hitboxHeight } };
        };
    auto MakeKnobRect = [&](float value, float y) -> UIRect {
        return UIRect{ { sliderLeft + sliderWidth * value, y }, { knobSize, knobSize } };
        };

    UIRect masterTrack = MakeTrackRect(masterY);
    UIRect bgmTrack = MakeTrackRect(bgmY);
    UIRect sfxTrack = MakeTrackRect(sfxY);
    UIRect masterKnob = MakeKnobRect(AudioManager::GetMasterVolume(), masterY);
    UIRect bgmKnob = MakeKnobRect(AudioManager::GetMusicVolume(), bgmY);
    UIRect sfxKnob = MakeKnobRect(AudioManager::GetSFXVolume(), sfxY);

    if (mousePressed)
    {
        if (PointInRectPx((float)mx, (float)my, masterTrack) || PointInRectPx((float)mx, (float)my, masterKnob))
            draggingMasterSlider = true;

        if (PointInRectPx((float)mx, (float)my, bgmTrack) || PointInRectPx((float)mx, (float)my, bgmKnob))
            draggingBgmSlider = true;

        if (PointInRectPx((float)mx, (float)my, sfxTrack) || PointInRectPx((float)mx, (float)my, sfxKnob))
            draggingSfxSlider = true;
    }

    if (!mouseHeld)
    {
        draggingMasterSlider = false;
        draggingBgmSlider = false;
        draggingSfxSlider = false;
    }

    if (draggingMasterSlider)
        AudioManager::SetMasterVolume(SliderValueFromMouse((float)mx, sliderLeft, sliderWidth));
    if (draggingBgmSlider)
        AudioManager::SetMusicVolume(SliderValueFromMouse((float)mx, sliderLeft, sliderWidth));
    if (draggingSfxSlider)
        AudioManager::SetSFXVolume(SliderValueFromMouse((float)mx, sliderLeft, sliderWidth));
}

void MainMenuScene::RenderMenuOverlay()
{
    DrawTextPx((s8)uiFontLarge, "SETTINGS", 40.f, 100.f, 0.6f, 1, 1, 1, 1);

    const float labelX = 420.0f;
    const float sliderLeft = 860.0f;
    const float sliderWidth = 330.0f;
    const float percentX = 1230.0f;
    const float masterY = 240.f;
    const float bgmY = 350.0f;
    const float sfxY = 460.0f;
    const float trackH = 8.0f;
    const float knobSz = 28.0f;

    auto DrawSlider = [&](const char* label, float value, float y, bool dragging)
        {
            int percent = (int)(value * 100.0f + 0.5f);
            DrawTextPx((s8)uiFont, label, labelX, y - 18.0f, 1.0f, 1, 1, 1, fadeSettingsAlpha);

            UIRect trackBg{ { sliderLeft + sliderWidth * 0.5f, y }, { sliderWidth, trackH } };
            DrawSolidPanel(trackBg, 0.20f * fadeSettingsAlpha);

            float filledW = sliderWidth * value;
            if (filledW > 0.0f)
            {
                UIRect trackFill{ { sliderLeft + filledW * 0.5f, y }, { filledW, trackH } };
                DrawSolidPanel(trackFill, 0.50f * fadeSettingsAlpha);
            }

            float knobX = sliderLeft + sliderWidth * value;
            UIRect knob{ { knobX, y }, { knobSz, knobSz } };
            DrawSolidPanel(knob, (dragging ? 0.60f : 0.36f) * fadeSettingsAlpha);

            DrawTextPx((s8)uiFont, std::to_string(percent), percentX, y - 18.0f, 1.0f, 1.0f, 0.95f, 0.35f, fadeSettingsAlpha);
        };

    DrawSlider("Master Volume", AudioManager::GetMasterVolume(), masterY, draggingMasterSlider);
    DrawSlider("BGM Volume", AudioManager::GetMusicVolume(), bgmY, draggingBgmSlider);
    DrawSlider("SFX Volume", AudioManager::GetSFXVolume(), sfxY, draggingSfxSlider);
}

void MainMenuScene::LoadRunRecords()
{
    personalBest = RunRecord{};
    latestRun = RunRecord{};

    std::ifstream in("Assets/Levels/leaderboard.txt");
    if (!in.is_open())
        in.open("../../Assets/Levels/leaderboard.txt");

    if (!in.is_open())
        return;

    std::string label;
    int levels = 0;
    double time = 0.0;

    while (in >> label >> levels >> time)
    {
        if (label == "BEST")
        {
            personalBest.levelsCleared = levels;
            personalBest.timeSeconds = time;
            personalBest.valid = true;
        }
        else if (label == "LATEST")
        {
            latestRun.levelsCleared = levels;
            latestRun.timeSeconds = time;
            latestRun.valid = true;
        }
    }
}

std::string MainMenuScene::FormatTime(double seconds) const
{
    if (seconds < 0.0)
        seconds = 0.0;

    int totalMs = static_cast<int>(seconds * 1000.0);
    int mm = (totalMs / 60000) % 100;
    int ss = (totalMs / 1000) % 60;
    int cs = (totalMs / 10) % 100;

    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << mm
        << ":"
        << std::setfill('0') << std::setw(2) << ss
        << ":"
        << std::setfill('0') << std::setw(2) << cs;
    return oss.str();
}

void MainMenuScene::RenderLeaderboard(float worldX, float worldY) const
{
    if (uiFont < 0)
        return;

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.f);
    AEGfxSetColorToMultiply(1.f, 1.f, 1.f, 1.f);
    AEGfxSetColorToAdd(0.f, 0.f, 0.f, 0.f);

    auto WorldToNDC = [](float wx, float wy, float& ndcX, float& ndcY)
        {
            float screenX = (wx - Camera::position.x) * Camera::scale + AEGfxGetWindowWidth() * 0.5f;
            float screenY = (wy - Camera::position.y) * Camera::scale + AEGfxGetWindowHeight() * 0.5f;
            ndcX = (screenX / AEGfxGetWindowWidth()) * 2.f - 1.f;
            ndcY = (screenY / AEGfxGetWindowHeight()) * 2.f - 1.f;
        };

    float nx, ny;
    float spacing = 0.5f;

    WorldToNDC(worldX, worldY, nx, ny);
    AEGfxPrint((s8)uiFont, "RUN RECORDS", nx, ny, 0.95f, 1.f, 0.9f, 0.45f, 1.f);

    QuickGraphics::DrawRect(worldX + 1.5f, worldY - 0.05f, 3.f, 0.05f, 0xFFFFE673); // Underline

    worldY -= spacing;
    if (personalBest.valid)
    {
        std::string best1 = "BEST LEVELS: " + std::to_string(personalBest.levelsCleared);
        std::string best2 = "BEST TIME:   " + FormatTime(personalBest.timeSeconds);

        WorldToNDC(worldX, worldY, nx, ny);
        AEGfxPrint((s8)uiFont, best1.c_str(), nx, ny, 0.72f, 1.f, 1.f, 1.f, 1.f);

        WorldToNDC(worldX, worldY - spacing, nx, ny);
        AEGfxPrint((s8)uiFont, best2.c_str(), nx, ny, 0.72f, 1.f, 1.f, 1.f, 1.f);
    }
    else
    {
        WorldToNDC(worldX, worldY, nx, ny);
        AEGfxPrint((s8)uiFont, "BEST LEVELS: --", nx, ny, 0.72f, 1.f, 1.f, 1.f, 1.f);

        WorldToNDC(worldX, worldY - spacing, nx, ny);
        AEGfxPrint((s8)uiFont, "BEST TIME:   --:--:--", nx, ny, 0.72f, 1.f, 1.f, 1.f, 1.f);
    }

    worldY -= 0.75f + spacing;
    if (latestRun.valid)
    {
        std::string latest1 = "LAST LEVELS: " + std::to_string(latestRun.levelsCleared);
        std::string latest2 = "LAST TIME:   " + FormatTime(latestRun.timeSeconds);

        WorldToNDC(worldX, worldY, nx, ny);
        AEGfxPrint((s8)uiFont, latest1.c_str(), nx, ny, 0.72f, 0.9f, 0.9f, 0.9f, 1.f);

        WorldToNDC(worldX, worldY - spacing, nx, ny);
        AEGfxPrint((s8)uiFont, latest2.c_str(), nx, ny, 0.72f, 0.9f, 0.9f, 0.9f, 1.f);
    }
    else
    {
        WorldToNDC(worldX, worldY, nx, ny);
        AEGfxPrint((s8)uiFont, "LAST LEVELS: --", nx, ny, 0.72f, 0.9f, 0.9f, 0.9f, 1.f);

        WorldToNDC(worldX, worldY - spacing, nx, ny);
        AEGfxPrint((s8)uiFont, "LAST TIME:   --:--:--", nx, ny, 0.72f, 0.9f, 0.9f, 0.9f, 1.f);
    }
}

void MainMenuScene::Init()
{
    SetCurrentDirectoryA(ExeDir().c_str());

    if (uiFont < 0)
    {
        uiFont = AEGfxCreateFont("Assets/buggy-font.ttf", 18);
        if (uiFont < 0) uiFont = AEGfxCreateFont("../Assets/buggy-font.ttf", 18);
        if (uiFont < 0) uiFont = AEGfxCreateFont("../../Assets/buggy-font.ttf", 18);

        uiFontLarge = AEGfxCreateFont("Assets/buggy-font.ttf", 42);
    }

    if (!fadeMesh)
    {
        AEGfxMeshStart();
        AEGfxTriAdd(-0.5f, -0.5f, 0xFF000000, 0.f, 0.f,
            0.5f, -0.5f, 0xFF000000, 0.f, 0.f,
            0.5f, 0.5f, 0xFF000000, 0.f, 0.f);
        AEGfxTriAdd(-0.5f, -0.5f, 0xFF000000, 0.f, 0.f,
            0.5f, 0.5f, 0xFF000000, 0.f, 0.f,
            -0.5f, 0.5f, 0xFF000000, 0.f, 0.f);
        fadeMesh = AEGfxMeshEnd();
    }

    isFadingToGame = false;
    fadeAlpha = 0.0f;
    LoadRunRecords();

    draggingMasterSlider = false;
    draggingBgmSlider = false;
    draggingSfxSlider = false;

    // Adjust these if needed after testing
    settingsTrigger = { 1.f, 25.f, 0.5f, 12.f };
    startGameTrigger = { 25.f, 30.f, 16.f, 27.f };

    vineTexture = AEGfxTextureLoad("Assets/Tmp/vines.png");
    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.f, 1.f,
        0.5f, -0.5f, 0xFFFFFFFF, 1.f, 1.f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.f, 0.f);
    AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.f, 1.f,
        0.5f, 0.5f, 0xFFFFFFFF, 1.f, 0.f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.f, 0.f);
    vineMesh = AEGfxMeshEnd();

    SpikePlate::LoadSharedRenderResources();

    const std::string path = "..\\..\\Assets\\Levels\\mainmenufrfr.lvl";

    LevelData lvl;
    RoomID startRoom = ROOM_1;

    roomMgr.Clear();
    roomSystem.ClearBlockedReturnDir();
    vinePositions.clear();

    if (LoadLevelFromFile(path.c_str(), lvl))
    {
        mapCols = lvl.cols;
        mapRows = lvl.rows;

        map.~MapGrid();
        new (&map) MapGrid(lvl.cols, lvl.rows);

        for (int y = 0; y < lvl.rows; ++y)
        {
            for (int x = 0; x < lvl.cols; ++x)
            {
                int v = lvl.tiles[(size_t)y * lvl.cols + x];
                if (v < 0 || v >= MapTile::typeCount)
                    v = 0;

                map.SetTile(x, y, (MapTile::Type)v);
            }
        }

        vinePositions = lvl.vines;

        BuildRoomsFromLevelData(lvl, roomMgr, startRoom);
        roomMgr.SetCurrentRoom(startRoom);
    }
    else
    {
        mapCols = ROOM_COLS;
        mapRows = ROOM_ROWS;

        map.~MapGrid();
        new (&map) MapGrid(mapCols, mapRows);

        RoomData room{};
        room.id = ROOM_1;
        room.gridX = 0;
        room.gridY = 0;
        room.startSpawn = { 2.5f, 2.5f };
        room.rightRoom = ROOM_NONE;

        for (int y = 0; y < ROOM_ROWS; ++y)
        {
            for (int x = 0; x < ROOM_COLS; ++x)
            {
                room.tiles[y][x] = MapTile::Type::NONE;
            }
        }

        for (int x = 0; x < ROOM_COLS; ++x)
        {
            room.tiles[0][x] = MapTile::Type::GROUND_BODY;
            room.tiles[1][x] = MapTile::Type::GROUND_SURFACE;

            map.SetTile(x, 0, MapTile::Type::GROUND_BODY);
            map.SetTile(x, 1, MapTile::Type::GROUND_SURFACE);
        }

        roomMgr.SetRoom(ROOM_1, room);
        roomMgr.SetCurrentRoom(ROOM_1);
    }

    camera.~Camera();
    new (&camera) Camera(
        { 0.f, 0.f },
        { static_cast<float>(mapCols), static_cast<float>(mapRows) },
        64.f
    );

    roomSystem.BuildCurrentRoom();
    camera.Update();
    AudioManager::Init();
    AudioManager::PlayMenuMusic();

    Background::Init();
}

void MainMenuScene::Update()
{
    if (IsMenuOpen())
    {
        UpdateMenuInput();
        AudioManager::Update();
    }

    const float dt = static_cast<float>(Time::GetInstance().GetScaledDeltaTime());

    if (isFadingToGame)
    {
        fadeAlpha += fadeSpeed * dt;
        if (fadeAlpha > 1.0f)
            fadeAlpha = 1.0f;

        if (fadeAlpha >= 1.0f)
        {
            GSM::ChangeScene(SceneState::GS_GAME);
            return;
        }

        return;
    }

    player.Update();

    if (IsPlayerInsideTrigger(settingsTrigger))
        fadeSettingsAlpha = min(fadeSettingsAlpha + fadeSpeed * dt, 1.f);
    else if (fadeSettingsAlpha > 0.f)
        fadeSettingsAlpha = max(fadeSettingsAlpha - fadeSpeed * dt, 0.f);
    
    if (IsPlayerInsideTrigger(startGameTrigger))
    {
        isFadingToGame = true;
        fadeAlpha = 0.0f;
        return;
    }

    trapMgr.Update(dt, player);
    enemyMgr.UpdateAll(player.GetPosition(), map);
    camera.Update();
    AudioManager::Update();
}

void MainMenuScene::Render()
{
    AEGfxSetBackgroundColor(0.15f, 0.15f, 0.15f);

    Background::Render();

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.f);
    AEGfxSetColorToMultiply(1.f, 1.f, 1.f, 1.f);
    AEGfxSetColorToAdd(0.f, 0.f, 0.f, 0.f);

    AEGfxSetCamPosition(Camera::position.x * Camera::scale,
        Camera::position.y * Camera::scale);

    map.Render();

    if (vineTexture && vineMesh)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1.f, 1.f, 1.f, 1.f);
        AEGfxSetColorToAdd(0.f, 0.f, 0.f, 0.f);
        AEGfxSetTransparency(1.f);
        AEGfxTextureSet(vineTexture, 0.f, 0.f);

        for (const auto& v : vinePositions)
        {
            AEMtx33 m;
            AEMtx33Trans(&m, v.x + 0.5f, v.y + 0.5f);
            AEMtx33ScaleApply(&m, &m, Camera::scale, Camera::scale);
            AEGfxSetTransform(m.m);
            AEGfxMeshDraw(vineMesh, AE_GFX_MDM_TRIANGLES);
        }
    }

    if (uiFont >= 0)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.f);
        AEGfxSetColorToMultiply(1.f, 1.f, 1.f, 1.f);
        AEGfxSetColorToAdd(0.f, 0.f, 0.f, 0.f);

        auto WorldToNDC = [](float wx, float wy, float& ndcX, float& ndcY)
            {
                float screenX = (wx - Camera::position.x) * Camera::scale + AEGfxGetWindowWidth() * 0.5f;
                float screenY = (wy - Camera::position.y) * Camera::scale + AEGfxGetWindowHeight() * 0.5f;
                ndcX = (screenX / AEGfxGetWindowWidth()) * 2.f - 1.f;
                ndcY = (screenY / AEGfxGetWindowHeight()) * 2.f - 1.f;
            };

        float nx, ny;

        WorldToNDC(8.4f, 25.f, nx, ny);
        AEGfxPrint((s8)uiFontLarge, "AETHERFALL", nx, ny, 1.f, 1.f, 1.f, 1.f, 1.f);

        WorldToNDC(4.1f, 15.5f, nx, ny);
        AEGfxPrint((s8)uiFont, "SETTINGS", nx, ny, 0.8f, 1.f, 0.82f, 0.35f, 1.f);

        WorldToNDC(3.3f, 26.5f, nx, ny);
        AEGfxPrint((s8)uiFont, "CREDITS", nx, ny, 0.9f, 1.f, 0.82f, 0.35f, 1.f);

        WorldToNDC(21.6f, 24.f, nx, ny);
        AEGfxPrint((s8)uiFont, "START QUEST >", nx, ny, 0.9f, 1.f, 0.82f, 0.35f, 1.f);
    }

    RenderLeaderboard(8.5f, 23.5f);

    if (fadeAlpha > 0.0f && fadeMesh)
    {
        AEGfxSetCamPosition(0.f, 0.f);
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(fadeAlpha);
        AEGfxSetColorToMultiply(1.f, 1.f, 1.f, 1.f);
        AEGfxSetColorToAdd(0.f, 0.f, 0.f, 0.f);

        AEMtx33 scale, transform;
        AEMtx33Scale(&scale, (float)AEGfxGetWindowWidth(), (float)AEGfxGetWindowHeight());
        transform = scale;

        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(fadeMesh, AE_GFX_MDM_TRIANGLES);
    }

    if (IsMenuOpen())
    {
        RenderMenuOverlay();
    }

    trapMgr.Render();
    player.Render();
    enemyMgr.RenderAll();

    if (Editor::GetShowColliders())
    {
        auto DrawTriggerCollider = [](const TriggerZone& trigger) {
            AEVec2 pos{ (trigger.maxX + trigger.minX) * 0.5f, (trigger.maxY + trigger.minY) * 0.5f };
            AEVec2 size{ trigger.maxX - trigger.minX, trigger.maxY - trigger.minY };
            QuickGraphics::DrawRect(pos, size, 0xFFFF0000, AE_GFX_MDM_LINES_STRIP);
        };

        DrawTriggerCollider(startGameTrigger);
        DrawTriggerCollider(settingsTrigger);
    }
}

void MainMenuScene::Exit()
{
    Time::GetInstance().SetPaused(false);

    if (vineTexture) { AEGfxTextureUnload(vineTexture); vineTexture = nullptr; }
    if (vineMesh) { AEGfxMeshFree(vineMesh); vineMesh = nullptr; }
    vinePositions.clear();

    TrapManager::UnloadAllSharedRenderResources();

    if (fadeMesh) { AEGfxMeshFree(fadeMesh); fadeMesh = nullptr; }

    if (uiFont >= 0)
    {
        AEGfxDestroyFont((s8)uiFont);
        uiFont = -1;
    }

    roomSystem.ClearRuntimeRoomObjects();
    roomMgr.Clear();
    AudioManager::Exit();
    Background::Exit();
}