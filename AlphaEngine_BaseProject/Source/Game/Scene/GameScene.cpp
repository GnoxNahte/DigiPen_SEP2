#include "GameScene.h"
#include "../../Utils/QuickGraphics.h"
#include "../../Utils/AEExtras.h"
#include "../Time.h"
#include "../../Game/UI.h"
#include "../../Game/Background.h"
#include "../BuffCards.h"
#include "LevelIO.h"
#include "../../Game/Timer.h"
#include <iomanip>
#include <sstream>
#include <fstream>
#include "../AudioManager.h"
#include "../Rooms/RoomBuilder.h"
#include "../enemy/AttackSystem.h"
#include "../../Utils/ScopedTimer.h"

std::string gPendingLevelPath = "Assets/Levels/gamescene.lvl";   // defined here, extern'd in MainMenuScene.cpp
std::string gLastLoadedLevelPath; // last successfully loaded level path for restart

namespace {
	bool gEndSequenceStarted = false;
}
/*void GameScene::ClampPlayerInsideCurrentRoom()
{
	if (roomMgr.GetCurrentRoomID() == ROOM_NONE)
		return;

	const AEVec2 origin = GetRoomOrigin(roomMgr.GetCurrentRoomID());
	AEVec2 p = player.GetPosition();

	// tune these values
	static constexpr float kLeftInset = 0.25f;
	static constexpr float kRightInset = 0.25f;
	static constexpr float kBottomInset = 1.f;
	static constexpr float kTopInset = 2.5f;

	const float minX = origin.x + kLeftInset;
	const float maxX = origin.x + (float)ROOM_COLS - 0.1f - kRightInset;
	const float minY = origin.y + kBottomInset;
	const float maxY = origin.y + (float)ROOM_ROWS - 0.1f - kTopInset;

	if (p.x < minX) p.x = minX;
	if (p.x > maxX) p.x = maxX;
	if (p.y < minY) p.y = minY;
	if (p.y > maxY) p.y = maxY;

	player.SetPosition(p);
}*/


GameScene::GameScene() :
	map(ROOM_COLS, ROOM_ROWS),
	player(&map, &enemyMgr),
	camera({ 1,1 }, { (float)(ROOM_COLS - 1), (float)(ROOM_ROWS - 1) }, 64),
	enemyBoss(35, 2.90f),
	roomSystem(map, player, camera, trapMgr, enemyMgr, enemyBoss, roomMgr)

{
	ScopedTimer timer("GameScene constructor");

	UI::Init(&player);
	Background::Init();
	AudioManager::Init();
	// Init pause overlay resources 
	pauseRectMesh = MeshGenerator::GetRectMesh(1.0f, 1.0f);
	pauseCardBackTex = AEGfxTextureLoad("Assets/Art/0_CardBack.png");

	// Load buff icon textures for pause overlay (same assets as BuffCardScreen)
	for (int i = 0; i < kPauseBuffTexCount; ++i) pauseBuffTex[i] = nullptr;

	// NOTE: These indices assume CARD_TYPE enum values are 0..N in this order.
	pauseBuffTex[(int)HERMES_FAVOR] = AEGfxTextureLoad("Assets/Art/Hermes_Favor.png");
	pauseBuffTex[(int)IRON_DEFENCE] = AEGfxTextureLoad("Assets/Art/Iron_Defence.png");
	pauseBuffTex[(int)SWITCH_IT_UP] = AEGfxTextureLoad("Assets/Art/Switch_It_Up.png");
	pauseBuffTex[(int)REVITALIZE] = AEGfxTextureLoad("Assets/Art/Revitalize.png");
	pauseBuffTex[(int)SHARPEN] = AEGfxTextureLoad("Assets/Art/Sharpen.png");
	pauseBuffTex[(int)BERSERKER] = AEGfxTextureLoad("Assets/Art/Berserker.png");
	pauseBuffTex[(int)FLEETING_STEP] = AEGfxTextureLoad("Assets/Art/Fleeting_Step.png");
	pauseBuffTex[(int)SUREFOOTED] = AEGfxTextureLoad("Assets/Art/Surefooted.png");
	pauseBuffTex[(int)DEEP_VITALITY] = AEGfxTextureLoad("Assets/Art/Deep_Vitality.png");
	pauseBuffTex[(int)HAND_OF_FATE] = AEGfxTextureLoad("Assets/Art/Hand_Of_Fate.png");
	pauseBuffTex[(int)SUNDERING_BLOW] = AEGfxTextureLoad("Assets/Art/Sundering_Blow.png");
	// Fonts for pause overlay
	pauseFontLarge = AEGfxCreateFont("Assets/m04.ttf", 55);
	pauseFontMedium = AEGfxCreateFont("Assets/m04.ttf", 50);
	pauseFontSmall = AEGfxCreateFont("Assets/m04.ttf", 35);
	pauseFontRuntime = AEGfxCreateFont("Assets/m04.ttf", 28);

	// Glow / emission textures (same as BuffCardScreen)
	for (int i = 0; i < kPauseRarityTexCount; ++i) pauseRarityTex[i] = nullptr;
	pauseRarityTex[RARITY_UNCOMMON] = AEGfxTextureLoad("Assets/Art/Uncommon_Emission.png");
	pauseRarityTex[RARITY_RARE] = AEGfxTextureLoad("Assets/Art/Rare_Emission.png");
	pauseRarityTex[RARITY_EPIC] = AEGfxTextureLoad("Assets/Art/Epic_Emission.png");
	pauseRarityTex[RARITY_LEGENDARY] = AEGfxTextureLoad("Assets/Art/Legendary_Emission.png");

	// Pixellari for description (match BuffCardScreen)
	pauseFontDesc = AEGfxCreateFont("Assets/Pixellari.ttf", 30);
}

GameScene::~GameScene()
{
	ScopedTimer timer("GameScene deconstructor");
	UI::Exit();
	Background::Exit();

	// Free pause overlay resources
	if (pauseRectMesh)
	{
		AEGfxMeshFree(pauseRectMesh);
		pauseRectMesh = nullptr;
	}
	if (pauseCardBackTex)
	{
		AEGfxTextureUnload(pauseCardBackTex);
		pauseCardBackTex = nullptr;
	}
	if (pauseFontLarge >= 0)
	{
		AEGfxDestroyFont(pauseFontLarge);
		pauseFontLarge = -1;
	}
	if (pauseFontSmall >= 0)
	{
		AEGfxDestroyFont(pauseFontSmall);
		pauseFontSmall = -1;
	}
	if (pauseFontRuntime >= 0)
	{
		AEGfxDestroyFont(pauseFontRuntime);
		pauseFontRuntime = -1;
	}

	// Free buff icon textures for pause overlay
	for (int i = 0; i < kPauseBuffTexCount; ++i)
	{
		if (pauseBuffTex[i])
		{
			AEGfxTextureUnload(pauseBuffTex[i]);
			pauseBuffTex[i] = nullptr;
		}
	}

	// Free rarity glow textures for pause overlay
	for (int i = 0; i < kPauseRarityTexCount; ++i)
	{
		if (pauseRarityTex[i])
		{
			AEGfxTextureUnload(pauseRarityTex[i]);
			pauseRarityTex[i] = nullptr;
		}
	}
	if (pauseFontDesc >= 0)
	{
		AEGfxDestroyFont(pauseFontDesc);
		pauseFontDesc = -1;
	}
	AudioManager::Exit();
	TrapManager::UnloadAllSharedRenderResources();
}

void GameScene::ResetRunRecordsForNewRun()
{
	currentRunLevelsCleared = 0;
	runRecorded = false;
}

void GameScene::OnLevelCleared()
{
	++currentRunLevelsCleared;
}

bool GameScene::IsBetterRun(const RunRecord& a, const RunRecord& b) const
{
	if (!a.valid)
		return false;

	if (!b.valid)
		return true;

	if (a.levelsCleared != b.levelsCleared)
		return a.levelsCleared > b.levelsCleared;

	return a.timeSeconds < b.timeSeconds;
}

void GameScene::LoadRunRecords()
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

void GameScene::SaveRunRecords() const
{
	std::ofstream out("Assets/Levels/leaderboard.txt", std::ios::trunc);
	if (!out.is_open())
		out.open("../../Assets/Levels/leaderboard.txt", std::ios::trunc);

	if (!out.is_open())
		return;

	if (personalBest.valid)
	{
		out << "BEST "
			<< personalBest.levelsCleared << ' '
			<< personalBest.timeSeconds << '\n';
	}

	if (latestRun.valid)
	{
		out << "LATEST "
			<< latestRun.levelsCleared << ' '
			<< latestRun.timeSeconds << '\n';
	}
}

void GameScene::FinalizeRunAndSave()
{
	if (runRecorded)
		return;

	RunRecord current;
	current.levelsCleared = currentRunLevelsCleared;
	current.timeSeconds = Time::GetInstance().GetScaledElapsedTime();
	current.valid = true;

	latestRun = current;

	if (IsBetterRun(current, personalBest))
	{
		personalBest = current;
	}

	SaveRunRecords();
	runRecorded = true;
}

void GameScene::Init()
{
	ScopedTimer timer("GameScene Init");

	Time::GetInstance().SetTimeScale(1.0f);
	LoadRunRecords();
	ResetRunRecordsForNewRun();
	gEndSequenceStarted = false;

	// clear room data from any prior run
	roomMgr.Clear();
	roomSystem.ClearBlockedReturnDir();
	enemyMgr.ResetAll();
	itemDropMgr.Init();
	itemDropMgr.Clear();

	bool loadedFromFile = false;
	LevelData loadedLevel{};
	RoomID startRoom = ROOM_1;

	const std::string pathToLoad =
		gPendingLevelPath.empty() ? "Assets/Levels/gamescene.lvl" : gPendingLevelPath;

	std::cout << "pathToLoad: " << pathToLoad << "\n";

	LevelData lvl;
	if (LoadLevelFromFile(pathToLoad.c_str(), lvl))
	{
		std::cout << "load success\n";
		std::cout << "loaded rows=" << lvl.rows << " cols=" << lvl.cols << "\n";

		loadedFromFile = true;
		loadedLevel = lvl;
		gLastLoadedLevelPath = pathToLoad;
		BuildRoomsFromLevelData(loadedLevel, roomMgr, startRoom);
	}
	else
	{
		std::cout << "load failed\n";
	}

	gPendingLevelPath.clear();

	if (!loadedFromFile)
	{
		LevelData fallback{};
		fallback.rows = ROOM_ROWS;
		fallback.cols = ROOM_COLS;
		fallback.spawn = { 2.5f, 3.0f };
		fallback.tiles.assign((size_t)ROOM_ROWS * (size_t)ROOM_COLS, (int)MapTile::Type::NONE);

		for (int x = 0; x < ROOM_COLS; ++x)
			fallback.tiles[(size_t)0 * ROOM_COLS + x] = (int)MapTile::Type::GROUND_BOTTOM;

		loadedLevel = fallback;
		BuildRoomsFromLevelData(loadedLevel, roomMgr, startRoom);
	}

	mapCols = loadedLevel.cols;
	mapRows = loadedLevel.rows;

	// rebuild full map from loaded level
	map.~MapGrid();
	new (&map) MapGrid(mapCols, mapRows);

	for (int y = 0; y < mapRows; ++y)
	{
		for (int x = 0; x < mapCols; ++x)
		{
			int v = loadedLevel.tiles[(size_t)y * mapCols + x];
			if (v < 0 || v >= MapTile::typeCount)
				v = 0;

			map.SetTile(x, y, (MapTile::Type)v);
		}
	}

	// rebuild camera using full level bounds
	camera.~Camera();
	new (&camera) Camera(
		{ 0.f, 0.f },
		{ (float)mapCols, (float)mapRows },
		64.0f
	);

	roomMgr.SetCurrentRoom(startRoom);
	roomSystem.BuildCurrentRoom();
	roomTransitionLocked = false;
	roomSystem.ClearBlockedReturnDir();
	roomInputLockTimer = 0.f;

	if (roomMgr.GetCurrentRoomID() == ROOM_1) {
		AudioManager::PlayGameMusic();
	}
}

void GameScene::Update()
{
	// Toggle pause with ESC (GameScene only)
	if (AEInputCheckTriggered(AEVK_ESCAPE))
	{
		// If we are inside sub-pages, ESC returns to menu instead of unpausing
		if (pausePage == PausePage::Settings || pausePage == PausePage::ConfirmQuit || pausePage == PausePage::ConfirmRestart) {
			pausePage = PausePage::Menu;
			//AudioManager::UnmuffleMusic();
		}
		else
		{
			//AudioManager::MuffleMusic();
			TogglePause();
		}
	}

	if (AEInputCheckTriggered(AEVK_9))
	{
		GSM::ChangeScene(SceneState::GS_LEVEL_EDITOR);
		return;
	}

	// When paused, skip gameplay update and only handle pause input
	if (IsPaused())
	{
		UpdatePauseInput();
		return;
	}


	if (UI::IsBossIntroActive())
	{
		UI::Update();
		camera.Update();
		return;
	}

	float dt = static_cast<float>(Time::GetInstance().GetScaledDeltaTime());

	roomSystem.Update(dt);

	// room transition input lock countdown
	if (roomInputLockTimer > 0.f)
	{
		roomInputLockTimer -= dt;
		if (roomInputLockTimer < 0.f)
			roomInputLockTimer = 0.f;
	}

	// only allow player input/update when not locked
	if (roomInputLockTimer <= 0.f)
	{
		player.Update();
		//roomSystem.ApplyStartRoomLeftBoundaryLock();
	}

#if _DEBUG
	if (AEInputCheckTriggered(AEVK_T))
	{
		roomMgr.SetCurrentRoom(ROOM_10);
		player.SetPosition({ 97, 30 });
	}
#endif

	// unlock only when:
	// 1) input lock finished
	// 2) player is no longer standing on an exit boundary
	if (roomTransitionLocked)
	{
		if (roomInputLockTimer <= 0.f && roomSystem.CheckRoomExit() == DIR_NONE)
			roomTransitionLocked = false;
	}

	if (!roomTransitionLocked)
	{
		
		RoomDirection exitDir = roomSystem.CheckRoomExit();
		if (exitDir != DIR_NONE)
		{
			// Block going back through the side we just entered from
			if (exitDir == roomSystem.GetBlockedReturnDir())
			{
				return;
			}

			const RoomID previousRoom = roomMgr.GetCurrentRoomID();
			const AEVec2 previousPos = player.GetPosition();

			if (roomMgr.ChangeRoom(exitDir))
			{
		
				OnLevelCleared();
				

				RoomDirection cameFrom = DIR_NONE;
				BuffCardManager::IsRoomCleared() = true;
				BuffCardScreen::ResetFlipSequence();

				switch (exitDir)
				{
				case DIR_TOP:    cameFrom = DIR_BOTTOM; break;
				case DIR_LEFT:   cameFrom = DIR_RIGHT;  break;
				case DIR_BOTTOM: cameFrom = DIR_TOP;    break;
				case DIR_RIGHT:  cameFrom = DIR_LEFT;   break;
				default: break;
				}

				const RoomID nextRoom = roomMgr.GetCurrentRoomID();
				const AEVec2 transitionSpawn = roomSystem.ComputeTransitionSpawn(previousRoom, nextRoom, previousPos);
				roomSystem.SetBlockedReturnDir(cameFrom);
				roomSystem.BuildCurrentRoom(cameFrom, &transitionSpawn);
				itemDropMgr.Clear();
				

				roomTransitionLocked = true;
				roomInputLockTimer = kRoomInputLockDuration;
				return;
			}
			else
			{
				//ClampPlayerInsideCurrentRoom();
				roomTransitionLocked = true;
				roomInputLockTimer = kRoomInputLockDuration;
				return;
			}
		}
	}

	camera.Update();

	//std::cout << player.GetPosition() << '\n';

	const AEVec2 pPos = player.GetPosition();
	const AEVec2 pSize = player.GetStats().playerSize;


	trapMgr.Update(dt, player);

	UI::GetDamageTextSpawner().Update();
	UI::Update();
	//std::cout << "MASTER VOL : " << AudioManager::GetMasterVolume()
	//		  << "BGM VOL : " << AudioManager::GetMusicVolume()
	//		  << "SFX VOL : " << AudioManager::GetSFXVolume() << '\n';
	//std::cout << "CURRENT ROOM : " << static_cast<int>(roomMgr.GetCurrentRoomID()) << '\n';
	if (roomMgr.GetCurrentRoomID() == ROOM_11) { // To change to ROOM_10 after spawning is done
		//std::cout << " IN BOSS ROOM !!!";
		AudioManager::gameMusic->Stop();
		AudioManager::PlayBossMusic(enemyBoss, roomMgr);
	}
	//std::cout << "Current room : " << static_cast<int>(roomMgr.GetCurrentRoomID()) << '\n';
	AudioManager::Update();

	if (!UI::isVictory && !player.IsDead()) // Only play lava music if not dead or victorious.
		AudioManager::UpdateLavaAudio(trapMgr, player);
	const bool endSequenceActive = player.IsDead() || UI::isVictory;

	if (endSequenceActive && !gEndSequenceStarted) {
		gEndSequenceStarted = true;
		FinalizeRunAndSave();

		AudioManager::trapLava->Stop();

		if (player.IsDead()) {
			AudioManager::PlayGameOverMusic();
		}
	}

	if (UI::GetRestartStatus()) { // Allow restart run from game over screen
		FinalizeRunAndSave();

		UI::GetRestartStatus() = false;
		pausePage = PausePage::None;

		Time::GetInstance().ResetElapsedTime();
		Time::GetInstance().SetTimeScale(1.0f);
		TimerSystem::GetInstance().Clear();
		UI::Reset();

		if (!BuffCardManager::GetCurrentBuffs().empty()) {
			BuffCardManager::ResetCurrentBuffs();
		}
		if (!gLastLoadedLevelPath.empty())
		{
			gPendingLevelPath = gLastLoadedLevelPath;
		}
		AudioManager::ResetForRestart();
		GSM::ChangeScene(SceneState::GS_GAME);
		return; // << This return stops the music from playing for a clean restart.
	}
	if (UI::GetReturnToMenuStatus()) {
		FinalizeRunAndSave();

		UI::GetReturnToMenuStatus() = false;
		pausePage = PausePage::None;
		Time::GetInstance().SetPaused(false);
		Time::GetInstance().SetTimeScale(1.0f);
		UI::Reset();
		GSM::ChangeScene(SceneState::GS_MAIN_MENU);
		return;
	}

	if (player.IsDead() || UI::isVictory) {
		return;
	}
	
	itemDropMgr.Update(player);
	enemyMgr.UpdateAll(pPos, player.GetIsFacingRight(), map);
	attackSystem.UpdateEnemyAttack(player, enemyMgr, roomSystem.GetActiveBoss());
}

void GameScene::Render()
{
	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetTransparency(1.f);
	AEGfxSetColorToMultiply(1.f, 1.f, 1.f, 1.f);
	AEGfxSetColorToAdd(0.f, 0.f, 0.f, 0.f);

	Background::Render();
	map.Render();
	trapMgr.Render();
	player.Render();
	if (roomSystem.GetActiveBoss())
		roomSystem.GetActiveBoss()->Render();
	//enemyBoss.Render();
	itemDropMgr.Render();
	enemyMgr.RenderAll();
	attackSystem.Render();
	UI::Render();
	if (UI::EndScreenContentVisible())
	{
		RenderEndScreenBuffs();
	}

	// in-game runtime HUD (top-right)
	if (!IsPaused() && !player.IsDead() && !UI::isVictory)
	{
		float w = (float)AEGfxGetWindowWidth();

		DrawTextPx(
			pauseFontRuntime,
			FormatRunTime(),
			w - 230.0f,   // top right corner with some margin
			50.0f,        // leave some margin from the top edge
			0.85f,        // smaller font size
			1.f, 1.f, 1.f, 1.f
		);
	}

	if (IsPaused())
	{
		RenderPauseOverlay();
	}

#if _DEBUG
	AEVec2 worldMousePos;
	AEExtras::GetCursorWorldPosition(worldMousePos);
	std::string str = "World Mouse Pos:" + std::to_string(worldMousePos.x) + ", " + std::to_string(worldMousePos.y);
	QuickGraphics::PrintText(str.c_str(), -1, 0.95f, 0.3f, 0.5f, 0.5f, 0.5f, 1);

	Vec2Int screenMousePos;
	AEInputGetCursorPosition(&screenMousePos.x, &screenMousePos.y);
	str = "Screen Mouse Pos:" + std::to_string(screenMousePos.x) + ", " + std::to_string(screenMousePos.y);
	QuickGraphics::PrintText(str.c_str(), -1, 0.90f, 0.3f, 0.5f, 0.5f, 0.5f, 1);

	str = "FPS:" + std::to_string(AEFrameRateControllerGetFrameRate());
	QuickGraphics::PrintText(str.c_str(), -1, 0.85f, 0.3f, 0.5f, 0.5f, 0.5f, 1);

	str = "Time:" + std::to_string(Time::GetInstance().GetScaledElapsedTime());
	QuickGraphics::PrintText(str.c_str(), -1, 0.80f, 0.3f, 0.5f, 0.5f, 0.5f, 1);

	std::string ppos = "Player Pos: " + std::to_string(player.GetPosition().x) + ", " + std::to_string(player.GetPosition().y);
	QuickGraphics::PrintText(ppos.c_str(), -1, 0.75f, 0.3f, 0.5f, 0.5f, 0.5f, 1);

	if (AEInputCheckTriggered(AEVK_R)) {
		FinalizeRunAndSave();

		pausePage = PausePage::None;
		Time::GetInstance().ResetElapsedTime();
		TimerSystem::GetInstance().Clear();
		UI::Reset();
		if (!BuffCardManager::GetCurrentBuffs().empty()) {
			BuffCardManager::ResetCurrentBuffs();
		}
		if (!gLastLoadedLevelPath.empty())
		{
			gPendingLevelPath = gLastLoadedLevelPath;
		}
		AudioManager::ResetForRestart();
		GSM::ChangeScene(SceneState::GS_GAME);
	}
#endif
}

void GameScene::Exit()
{
	pausePage = PausePage::None;
	Time::GetInstance().SetPaused(false);
}

bool GameScene::IsPaused() const
{
	return pausePage != PausePage::None;
}

void GameScene::TogglePause()
{
	if (pausePage == PausePage::None)
		pausePage = PausePage::Menu;
	else
		pausePage = PausePage::None;

	Time::GetInstance().SetPaused(IsPaused());
}

static AEVec2 ScreenToEngine(float px, float py)
{
	float w = (float)AEGfxGetWindowWidth();
	float h = (float)AEGfxGetWindowHeight();
	return AEVec2{ px - w * 0.5f, (h * 0.5f) - py };
}

void GameScene::DrawDimBackground(float alpha)
{
	if (alpha <= 0.0f) return;

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
	AEGfxMeshDraw(pauseRectMesh, AE_GFX_MDM_TRIANGLES);
}

void GameScene::DrawSolidPanel(const UIRect& r, float alpha)
{
	AEMtx33 scale, rot, trans, transform;
	AEMtx33Scale(&scale, r.size.x, r.size.y);
	AEMtx33Rot(&rot, 0.0f);

	AEVec2 eng = ScreenToEngine(r.pos.x, r.pos.y);
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
	AEGfxMeshDraw(pauseRectMesh, AE_GFX_MDM_TRIANGLES);
}

void GameScene::DrawTexturePanel(AEGfxTexture* tex, const UIRect& r, float alpha)
{
	if (!tex) return;

	AEMtx33 scale, rot, trans, transform;
	AEMtx33Scale(&scale, r.size.x, r.size.y);
	AEMtx33Rot(&rot, 0.0f);

	AEVec2 eng = ScreenToEngine(r.pos.x, r.pos.y);
	AEMtx33Trans(&trans,
		eng.x + Camera::position.x * Camera::scale,
		eng.y + Camera::position.y * Camera::scale);

	AEMtx33Concat(&transform, &rot, &scale);
	AEMtx33Concat(&transform, &trans, &transform);

	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
	AEGfxSetColorToMultiply(1.f, 1.f, 1.f, 1.f);
	AEGfxSetColorToAdd(0.f, 0.f, 0.f, 0.f);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetTransparency(alpha);

	AEGfxTextureSet(tex, 0, 0);
	AEGfxSetTransform(transform.m);
	AEGfxMeshDraw(pauseRectMesh, AE_GFX_MDM_TRIANGLES);
}

void GameScene::DrawTextPx(s8 font, const std::string& text, float px, float py, float scale, float r, float g, float b, float a)
{
	float w = (float)AEGfxGetWindowWidth();
	float h = (float)AEGfxGetWindowHeight();

	float xNdc = (px / w) * 2.0f - 1.0f;
	float yNdc = 1.0f - (py / h) * 2.0f;

	AEGfxPrint(font, text.c_str(), xNdc, yNdc, scale, r, g, b, a);
}

bool GameScene::IsMouseOver(const UIRect& r) const
{
	return Button::CheckMouseInRectButton(r.pos, r.size);
}

bool GameScene::IsClicked(const UIRect& r) const
{
	return IsMouseOver(r) && AEInputCheckTriggered(AEVK_LBUTTON);
}

std::string GameScene::FormatRunTime() const
{
	double t = Time::GetInstance().GetScaledElapsedTime();
	int totalMs = (int)(t * 1000.0);

	int mm = (totalMs / 60000) % 100;
	int ss = (totalMs / 1000) % 60;
	int cs = (totalMs / 10) % 100;

	std::ostringstream oss;
	oss << std::setfill('0') << std::setw(2) << mm << ":"
		<< std::setfill('0') << std::setw(2) << ss << ":"
		<< std::setfill('0') << std::setw(2) << cs;
	return oss.str();
}

void GameScene::UpdatePauseInput()
{
	UIRect btnResume{ {150, 300}, {340, 60} };
	UIRect btnRestart{ {150, 400}, {340, 60} };
	UIRect btnSettings{ {150, 500}, {340, 60} };
	UIRect btnMenu{ {150, 600}, {340, 60} };

	if (pausePage == PausePage::Menu)
	{
		if (IsClicked(btnResume))
		{
			AudioManager::PlayButtonClick();
			TogglePause();
			AudioManager::UnmuffleMusic();
			return;
		}
		if (IsClicked(btnRestart))
		{
			AudioManager::PlayButtonClick();
			pausePage = PausePage::ConfirmRestart;
			return;
		}
		if (IsClicked(btnSettings))
		{
			AudioManager::PlayButtonClick();
			pausePage = PausePage::Settings;
			return;
		}
		if (IsClicked(btnMenu))
		{
			AudioManager::PlayButtonClick();
			pausePage = PausePage::ConfirmQuit;
			return;
		}
	}
	else if (pausePage == PausePage::ConfirmQuit)
	{
		float w = (float)AEGfxGetWindowWidth();
		float h = (float)AEGfxGetWindowHeight();
		float centerX = w * 0.5f;
		float centerY = h * 0.5f;

		UIRect btnNo{ { centerX - 160.0f, centerY + 80.0f }, {180.0f, 60.0f} };
		UIRect btnYes{ { centerX + 160.0f, centerY + 80.0f }, {180.0f, 60.0f} };

		if (IsClicked(btnNo))
		{
			AudioManager::PlayButtonClick();
			pausePage = PausePage::Menu;
			return;
		}
		if (IsClicked(btnYes))
		{
			AudioManager::PlayButtonClick();
			Sleep(150);
			FinalizeRunAndSave();

			pausePage = PausePage::None;
			Time::GetInstance().SetPaused(false);
			Time::GetInstance().ResetElapsedTime();
			Time::GetInstance().SetTimeScale(1.0f);
			TimerSystem::GetInstance().Clear();
			UI::Reset();
			if (!BuffCardManager::GetCurrentBuffs().empty())
			{
				BuffCardManager::ResetCurrentBuffs();
			}

			if (!gLastLoadedLevelPath.empty())
			{
				gPendingLevelPath = gLastLoadedLevelPath;
			}

			AudioManager::ResetForRestart();
			GSM::ChangeScene(SceneState::GS_MAIN_MENU);
			return;
		}
	}
	else if (pausePage == PausePage::ConfirmRestart)
	{
		float w = (float)AEGfxGetWindowWidth();
		float h = (float)AEGfxGetWindowHeight();
		float centerX = w * 0.5f;
		float centerY = h * 0.5f;

		UIRect btnNo{ { centerX - 160.0f, centerY + 80.0f }, {180.0f, 60.0f} };
		UIRect btnYes{ { centerX + 160.0f, centerY + 80.0f }, {180.0f, 60.0f} };

		if (IsClicked(btnNo))
		{
			AudioManager::PlayButtonClick();
			pausePage = PausePage::Menu;
			return;
		}
		if (IsClicked(btnYes))
		{
			AudioManager::PlayButtonClick();
			Sleep(150); // small delay to allow button click sound to play before restarting
			FinalizeRunAndSave();

			pausePage = PausePage::None;
			Time::GetInstance().SetPaused(false);
			Time::GetInstance().ResetElapsedTime();
			Time::GetInstance().SetTimeScale(1.0f);
			TimerSystem::GetInstance().Clear();
			UI::Reset();

			if (!BuffCardManager::GetCurrentBuffs().empty())
			{
				BuffCardManager::ResetCurrentBuffs();
			}

			if (!gLastLoadedLevelPath.empty())
			{
				gPendingLevelPath = gLastLoadedLevelPath;
			}

			AudioManager::ResetForRestart();
			GSM::ChangeScene(SceneState::GS_GAME);
			return;
		}
	}
	else if (pausePage == PausePage::Settings)
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
}

void GameScene::RenderBuffSummaryGrid(float anchorX, float anchorY, const std::string& title, int cols)
{
	const auto& buffs = BuffCardManager::GetCurrentBuffs();
	if (buffs.empty())
		return;

	float w = (float)AEGfxGetWindowWidth();
	float h = (float)AEGfxGetWindowHeight();

	const float cardW = 130.0f;
	const float cardH = 185.0f;
	const float gapX = 35.0f;
	const float gapY = 20.0f;

	DrawTextPx(pauseFontLarge, title, anchorX, anchorY - 30.0f, 0.55f, 1, 0.2f, 0.85f, 1);

	for (int i = 0; i < (int)buffs.size(); ++i)
	{
		const BuffCard& b = buffs[i];

		UIRect card;
		card.size = { cardW, cardH };

		const int cx = i % cols;
		const int cy = i / cols;

		const float centerX = anchorX + cx * (cardW + gapX) + cardW * 0.5f;
		const float centerY = anchorY + cy * (cardH + gapY) + cardH * 0.5f;
		card.pos = { centerX, centerY };

		AEGfxTexture* tex = nullptr;
		int typeIdx = (int)b.type;
		if (typeIdx >= 0 && typeIdx < kPauseBuffTexCount)
			tex = pauseBuffTex[typeIdx];
		if (!tex)
			tex = pauseCardBackTex;

		DrawTexturePanel(tex, card, 1.0f);

		AEGfxTexture* glow = nullptr;
		int rarityIdx = (int)b.rarity;
		if (rarityIdx >= 0 && rarityIdx < kPauseRarityTexCount)
			glow = pauseRarityTex[rarityIdx];

		if (glow)
		{
			const float EMISSION_SCALE = 1.15f;
			UIRect glowRect = card;
			glowRect.size.x *= EMISSION_SCALE;
			glowRect.size.y *= EMISSION_SCALE;
			DrawTexturePanel(glow, glowRect, 1.0f);
		}

		if (IsMouseOver(card))
		{
			DrawSolidPanel(UIRect{ { w * 0.5f, h - 90.0f }, { w * 0.85f, 90.0f } }, 0.55f);

			f32 red{}, green{}, blue{};
			switch (b.rarity)
			{
			case RARITY_UNCOMMON:
				red = 0.015f; green = 1.0f;   blue = 0.0f;
				break;
			case RARITY_RARE:
				red = 0.0f;   green = 0.384f; blue = 1.0f;
				break;
			case RARITY_EPIC:
				red = 0.584f; green = 0.0f;   blue = 1.0f;
				break;
			case RARITY_LEGENDARY:
				red = 1.0f;   green = 0.733f; blue = 0.0f;
				break;
			default:
				red = green = blue = 1.0f;
				break;
			}

			DrawTextPx(
				pauseFontSmall,
				b.cardName,
				120.0f, h - 125.0f, 1.0f,
				red, green, blue, 1.0f
			);

			const std::string desc = b.cardEffect.empty() ? b.cardDesc : b.cardEffect;

			DrawTextPx(
				pauseFontDesc,
				desc,
				120.0f, h - 65.0f, 1.0f,
				0.9f, 0.9f, 0.9f, 1.0f
			);
		}
	}
}

void GameScene::RenderEndScreenBuffs()
{
	float w = (float)AEGfxGetWindowWidth();

	// 先放右侧，尽量不挡住中间的 victory / defeat 文字和按钮
	RenderBuffSummaryGrid(w * 0.56f, 220.0f, "BUFFS ATTAINED:", 4);
}

void GameScene::RenderPauseOverlay()
{
	float w = (float)AEGfxGetWindowWidth();
	float h = (float)AEGfxGetWindowHeight();

	DrawDimBackground(0.75f);

	if (pausePage != PausePage::Settings)
	{
		DrawTextPx(pauseFontLarge, "PAUSED", 40, 100, 1.0f, 1, 1, 1, 1);
		DrawTextPx(pauseFontRuntime, "Run Time : " + FormatRunTime(), 40, 150, 1.0f, 1, 1, 1, 1);
	}

	auto drawBtn = [&](const char* label, const UIRect& r)
		{
			const bool hover = IsMouseOver(r);
			const float textScale = hover ? 1.05f : 1.0f;
			float cr = 1.f, cg = 1.f, cb = 1.f, ca = hover ? 1.f : 0.85f;
			if (hover) { cr = 1.0f; cg = 0.95f; cb = 0.35f; }
			DrawTextPx(pauseFontSmall, label, r.pos.x - 55, r.pos.y + 6, textScale, cr, cg, cb, ca);
		};

	UIRect btnResume{ {150, 300}, {340, 60} };
	UIRect btnRestart{ {150, 400}, {340, 60} };
	UIRect btnSettings{ {150, 500}, {340, 60} };
	UIRect btnMenu{ {150, 600}, {340, 60} };

	if (pausePage == PausePage::Menu)
	{
		drawBtn("Resume", btnResume);
		drawBtn("Restart Run", btnRestart);
		drawBtn("Settings", btnSettings);
		drawBtn("Menu", btnMenu);
	}
	else if (pausePage == PausePage::Settings)
	{
		DrawTextPx(pauseFontLarge, "SETTINGS", 40.f, 100.f, 1.0f, 1, 1, 1, 1);
		DrawTextPx(pauseFontRuntime, "ESC - BACK", 40.f, 845.f, 1.0f, 1, 1, 1, 1);

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
				DrawTextPx(pauseFontRuntime, label, labelX, y - 18.0f, 1.0f, 1, 1, 1, 1);

				UIRect trackBg{ { sliderLeft + sliderWidth * 0.5f, y }, { sliderWidth, trackH } };
				DrawSolidPanel(trackBg, 0.20f);

				float filledW = sliderWidth * value;
				if (filledW > 0.0f)
				{
					UIRect trackFill{ { sliderLeft + filledW * 0.5f, y }, { filledW, trackH } };
					DrawSolidPanel(trackFill, 0.50f);
				}

				float knobX = sliderLeft + sliderWidth * value;
				UIRect knob{ { knobX, y }, { knobSz, knobSz } };
				DrawSolidPanel(knob, dragging ? 0.60f : 0.36f);

				DrawTextPx(pauseFontRuntime, std::to_string(percent), percentX, y - 18.0f, 1.0f, 1.0f, 0.95f, 0.35f, 1.0f);
			};
		DrawSlider("Master Volume", AudioManager::GetMasterVolume(), masterY, draggingMasterSlider);
		DrawSlider("BGM Volume", AudioManager::GetMusicVolume(), bgmY, draggingBgmSlider);
		DrawSlider("SFX Volume", AudioManager::GetSFXVolume(), sfxY, draggingSfxSlider);
	}
	else if (pausePage == PausePage::ConfirmQuit)
	{
		float centerX = w * 0.5f;
		float centerY = h * 0.5f;

		{
			AEMtx33 scale, rot, trans, transform;
			AEMtx33Scale(&scale, 850.0f, 400.0f);
			AEMtx33Rot(&rot, 0.0f);
			AEVec2 eng = ScreenToEngine(centerX, centerY);
			AEMtx33Trans(&trans, eng.x + Camera::position.x * Camera::scale, eng.y + Camera::position.y * Camera::scale);
			AEMtx33Concat(&transform, &rot, &scale);
			AEMtx33Concat(&transform, &trans, &transform);

			AEGfxSetRenderMode(AE_GFX_RM_COLOR);
			AEGfxSetColorToMultiply(0.f, 0.f, 0.f, 0.f);
			AEGfxSetColorToAdd(0.18f, 0.18f, 0.18f, 0.90f);
			AEGfxSetBlendMode(AE_GFX_BM_BLEND);
			AEGfxSetTransparency(0.90f);
			AEGfxSetTransform(transform.m);
			AEGfxMeshDraw(pauseRectMesh, AE_GFX_MDM_TRIANGLES);
		}

		DrawTextPx(pauseFontMedium, "RETURN TO MENU?", centerX - 380.0f, centerY - 100.0f, 1.0f, 1, 1, 1, 1);
		DrawTextPx(pauseFontSmall, "PROGRESS WILL BE LOST", centerX - 370.0f, centerY - 20.0f, 1.0f, 0.8f, 0.8f, 0.8f, 1);

		UIRect btnNo{ { centerX - 160.0f, centerY + 80.0f }, {180.0f, 60.0f} };
		UIRect btnYes{ { centerX + 160.0f, centerY + 80.0f }, {180.0f, 60.0f} };

		bool hoverNo = IsMouseOver(btnNo);
		float scaleNo = hoverNo ? 1.05f : 1.0f;
		float rNo = 1.f, gNo = 1.f, bNo = 1.f, aNo = hoverNo ? 1.f : 0.85f;
		if (hoverNo) { rNo = 1.0f; gNo = 0.95f; bNo = 0.35f; }

		bool hoverYes = IsMouseOver(btnYes);
		float scaleYes = hoverYes ? 1.05f : 1.0f;
		float rYes = 1.f, gYes = 1.f, bYes = 1.f, aYes = hoverYes ? 1.f : 0.85f;
		if (hoverYes) { rYes = 1.0f; gYes = 0.4f; bYes = 0.4f; }

		DrawTextPx(pauseFontSmall, "NO", btnNo.pos.x - 35.0f, btnNo.pos.y + 12.0f, scaleNo, rNo, gNo, bNo, aNo);
		DrawTextPx(pauseFontSmall, "YES", btnYes.pos.x - 52.0f, btnYes.pos.y + 12.0f, scaleYes, rYes, gYes, bYes, aYes);
	}
	else if (pausePage == PausePage::ConfirmRestart)
	{
		float centerX = w * 0.5f;
		float centerY = h * 0.5f;

		{
			AEMtx33 scale, rot, trans, transform;
			AEMtx33Scale(&scale, 850.0f, 400.0f);
			AEMtx33Rot(&rot, 0.0f);
			AEVec2 eng = ScreenToEngine(centerX, centerY);
			AEMtx33Trans(&trans, eng.x + Camera::position.x * Camera::scale, eng.y + Camera::position.y * Camera::scale);
			AEMtx33Concat(&transform, &rot, &scale);
			AEMtx33Concat(&transform, &trans, &transform);

			AEGfxSetRenderMode(AE_GFX_RM_COLOR);
			AEGfxSetColorToMultiply(0.f, 0.f, 0.f, 0.f);
			AEGfxSetColorToAdd(0.18f, 0.18f, 0.18f, 0.90f);
			AEGfxSetBlendMode(AE_GFX_BM_BLEND);
			AEGfxSetTransparency(0.90f);
			AEGfxSetTransform(transform.m);
			AEGfxMeshDraw(pauseRectMesh, AE_GFX_MDM_TRIANGLES);
		}

		DrawTextPx(pauseFontLarge, "RESTART RUN?", centerX - 320.0f, centerY - 100.0f, 1.0f, 1, 1, 1, 1);
		DrawTextPx(pauseFontSmall, "PROGRESS WILL BE LOST", centerX - 370.0f, centerY - 20.0f, 1.0f, 0.8f, 0.8f, 0.8f, 1);

		UIRect btnNo{ { centerX - 160.0f, centerY + 80.0f }, {180.0f, 60.0f} };
		UIRect btnYes{ { centerX + 160.0f, centerY + 80.0f }, {180.0f, 60.0f} };

		bool hoverNo = IsMouseOver(btnNo);
		float scaleNo = hoverNo ? 1.05f : 1.0f;
		float rNo = 1.f, gNo = 1.f, bNo = 1.f, aNo = hoverNo ? 1.f : 0.85f;
		if (hoverNo) { rNo = 1.0f; gNo = 0.95f; bNo = 0.35f; }

		bool hoverYes = IsMouseOver(btnYes);
		float scaleYes = hoverYes ? 1.05f : 1.0f;
		float rYes = 1.f, gYes = 1.f, bYes = 1.f, aYes = hoverYes ? 1.f : 0.85f;
		if (hoverYes) { rYes = 1.0f; gYes = 0.4f; bYes = 0.4f; }

		DrawTextPx(pauseFontSmall, "NO", btnNo.pos.x - 35.0f, btnNo.pos.y + 12.0f, scaleNo, rNo, gNo, bNo, aNo);
		DrawTextPx(pauseFontSmall, "YES", btnYes.pos.x - 52.0f, btnYes.pos.y + 12.0f, scaleYes, rYes, gYes, bYes, aYes);
	}
	
	if (pausePage == PausePage::Menu)
	{
		RenderBuffSummaryGrid(w * 0.56f, 110.0f, "ACTIVE BUFFS:", 4);
	}

}