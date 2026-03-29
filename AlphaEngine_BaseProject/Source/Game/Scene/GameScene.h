#pragma once
#include "../Player/Player.h"
#include "../Camera.h"
#include "../Environment/MapGrid.h"
#include "../../Utils/ParticleSystem.h"
#include "../Environment/traps.h"
#include "GSM.h"
#include "../enemy/EnemyBoss.h"
#include "../enemy/Enemy.h"
#include "../enemy/EnemyManager.h"
#include "../enemy/AttackSystem.h"
#include "../../Game/Rooms/RoomManager.h"
#include "../Rooms/RoomBuilder.h"
#include "../Rooms/RoomSystem.h"
#include <string>

class GameScene : public BaseScene
{
public:
	GameScene();
	~GameScene();
	void Init() override;
	void Update() override;
	void Render() override;
	void Exit() override;
private:
	MapGrid map;
	Player player;
	Camera camera;
	//EnemyA enemyA;
	//EnemyB enemyB;
	EnemyBoss enemyBoss;
	EnemyBoss* activeBoss = nullptr;

	TrapManager trapMgr;

	Enemy enemyA;
	Enemy enemyB;

	EnemyManager enemyMgr;
	AttackSystem attackSystem;
	RoomManager roomMgr;
	RoomSystem roomSystem;
	bool roomTransitionLocked = false;
	bool draggingMasterSlider = false;
	bool draggingBgmSlider = false;
	bool draggingSfxSlider = false;

	// To prevent accidental double-transitioning when player hits a door, we lock input for a short duration after a room transition
	static constexpr float kRoomInputLockDuration = 0.10f;
	float roomInputLockTimer = 0.f;

	// ======================= Pause Overlay (No ImGui) =======================
	enum class PausePage
	{
		None,
		Menu,
		Settings,
		ConfirmQuit,
		ConfirmRestart
	};

	PausePage pausePage = PausePage::None;

	// UI resources
	AEGfxVertexList* pauseRectMesh = nullptr;
	AEGfxTexture* pauseCardBackTex = nullptr;
	s8 pauseFontLarge = -1;
	s8 pauseFontMedium = -1;
	s8 pauseFontSmall = -1;
	s8 pauseFontRuntime = -1; // for dynamic text like run time

	// Simple UI rectangle in screen pixels (pos is center)
	struct UIRect
	{
		AEVec2 pos;   // screen pixels, center-based
		AEVec2 size;  // width/height in pixels
	};

	// Pause helpers
	bool IsPaused() const;
	void TogglePause();
	void UpdatePauseInput();
	void RenderPauseOverlay();

	// Draw helpers (no ImGui)
	void DrawDimBackground(float alpha);
	void DrawSolidPanel(const UIRect& r, float alpha);
	void DrawTexturePanel(AEGfxTexture* tex, const UIRect& r, float alpha);
	void DrawTextPx(s8 font, const std::string& text, float px, float py, float scale, float r, float g, float b, float a);
	bool IsMouseOver(const UIRect& r) const;
	bool IsClicked(const UIRect& r) const;

	// UI layout helpers
	std::string FormatRunTime() const;
	// ---- Pause overlay textures (buff icons) ----
	static constexpr int kPauseBuffTexCount = 20; // enough for your CARD_TYPE values
	AEGfxTexture* pauseBuffTex[kPauseBuffTexCount] = { nullptr };

	// glowing rarity overlay textures for cards (same as BuffCardScreen)
	static constexpr int kPauseRarityTexCount = 4; // UNCOMMON/RARE/EPIC/LEGENDARY
	AEGfxTexture* pauseRarityTex[kPauseRarityTexCount] = { nullptr };
	s8 pauseFontDesc = -1; // Pixellari

	int mapCols = ROOM_COLS;
	int mapRows = ROOM_ROWS;

	// ======================= Run Record / Leaderboard Sync =======================
	struct RunRecord
	{
		int levelsCleared = 0;
		double timeSeconds = 0.0;
		bool valid = false;
	};

	int currentRunLevelsCleared = 0;
	bool runRecorded = false;

	RunRecord personalBest;
	RunRecord latestRun;

	void ResetRunRecordsForNewRun();
	void OnLevelCleared();
	void FinalizeRunAndSave();
	void LoadRunRecords();
	void SaveRunRecords() const;
	bool IsBetterRun(const RunRecord& a, const RunRecord& b) const;
};