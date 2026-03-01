#pragma once
#include "../Player/Player.h"
#include "../Camera.h"
#include <cmath>
#include "../Environment/MapGrid.h"
#include "../../Utils/ParticleSystem.h"
#include "../../Utils/MeshGenerator.h"
#include "../Environment/traps.h"
#include "GSM.h"
#include "../enemy/EnemyBoss.h"
#include "../enemy/Enemy.h"
#include "../enemy/EnemyManager.h"
#include "../enemy/AttackSystem.h"



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

	ParticleSystem testParticleSystem;
	TrapManager trapMgr;

	Enemy enemyA;
	Enemy enemyB;

	EnemyManager enemyMgr;
	AttackSystem attackSystem;
	// ======================= Pause Overlay (No ImGui) =======================
	enum class PausePage
	{
		None,
		Menu,
		Settings,
		ConfirmQuit
	};

	PausePage pausePage = PausePage::None;

	// UI resources
	AEGfxVertexList* pauseRectMesh = nullptr;
	AEGfxTexture* pauseCardBackTex = nullptr;
	s8 pauseFontLarge = -1;
	s8 pauseFontSmall = -1;

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
	static constexpr int kPauseBuffTexCount = 8; // enough for your CARD_TYPE values
	AEGfxTexture* pauseBuffTex[kPauseBuffTexCount] = { nullptr };

	// glowing rarity overlay textures for cards (same as BuffCardScreen)
	static constexpr int kPauseRarityTexCount = 4; // UNCOMMON/RARE/EPIC/LEGENDARY
	AEGfxTexture* pauseRarityTex[kPauseRarityTexCount] = { nullptr };
	s8 pauseFontDesc = -1; // Pixellari
};

