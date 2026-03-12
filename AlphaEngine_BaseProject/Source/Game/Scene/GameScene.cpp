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

std::string gPendingLevelPath;   // defined here, extern'd in MainMenuScene.cpp
std::string gLastLoadedLevelPath; // last successfully loaded level path for restart

//AABB collision helper
/*static bool AABB_Overlap(const AEVec2& aPos, const AEVec2& aSize,
	const AEVec2& bPos, const AEVec2& bSize)
{
	const float dx = std::fabs(aPos.x - bPos.x);
	const float dy = std::fabs(aPos.y - bPos.y);
	return dx <= (aSize.x + bSize.x) * 0.5f
		&& dy <= (aSize.y + bSize.y) * 0.5f;
}*/


namespace
{
	// squared-distance check (no sqrt)
	bool IsNear(const AEVec2& a, const AEVec2& b, float range)
	{
		float dx = b.x - a.x;
		float dy = b.y - a.y;
		return (dx * dx + dy * dy) <= (range * range);
	}

	void BuildTestRooms(RoomManager& roomMgr)
	{
		roomMgr.Clear();

		// =========================
		// ROOM 1
		// layout:
		// ROOM_3
		//   |
		// ROOM_1 --- ROOM_2
		// =========================
		RoomData room1{};
		room1.id = ROOM_1;
		room1.rightRoom = ROOM_2;
		room1.topRoom = ROOM_3;

		room1.startSpawn = { 2.5f, 3.0f };
		room1.entryFromLeft = { 3.0f, 3.0f };
		room1.entryFromRight = { 21.0f, 3.0f };
		room1.entryFromTop = { 12.5f, 10.0f };
		room1.entryFromBottom = { 12.5f, 3.0f };

		// floor
		for (int x = 0; x < ROOM_COLS; ++x)
			room1.tiles[0][x] = MapTile::Type::GROUND_BOTTOM;

		// little platform
		for (int x = 8; x <= 12; ++x)
			room1.tiles[4][x] = MapTile::Type::GROUND_SURFACE;

		// left wall
		for (int y = 1; y <= 5; ++y)
			room1.tiles[y][0] = MapTile::Type::GROUND_BODY;

		// =========================
		// ROOM 2
		// =========================
		RoomData room2{};
		room2.id = ROOM_2;
		room2.leftRoom = ROOM_1;

		room2.startSpawn = { 2.5f, 3.0f };
		room2.entryFromLeft = { 3.0f, 3.0f };
		room2.entryFromRight = { 21.0f, 3.0f };
		room2.entryFromTop = { 12.5f, 10.0f };
		room2.entryFromBottom = { 12.5f, 3.0f };

		for (int x = 0; x < ROOM_COLS; ++x)
			room2.tiles[0][x] = MapTile::Type::GROUND_BOTTOM;

		// raised platform
		for (int x = 14; x <= 20; ++x)
			room2.tiles[5][x] = MapTile::Type::GROUND_SURFACE;

		// one enemy
		room2.enemyCount = 1;
		room2.enemies[0].preset = (int)EnemySpawnType::Druid;
		room2.enemies[0].pos = { 16.5f, 2.0f };

		// =========================
		// ROOM 3
		// =========================
		RoomData room3{};
		room3.id = ROOM_3;
		room3.bottomRoom = ROOM_1;

		room3.startSpawn = { 12.5f, 3.0f };
		room3.entryFromLeft = { 3.0f, 3.0f };
		room3.entryFromRight = { 21.0f, 3.0f };
		room3.entryFromTop = { 12.5f, 10.0f };
		room3.entryFromBottom = { 12.5f, 3.0f };

		for (int x = 0; x < ROOM_COLS; ++x)
			room3.tiles[0][x] = MapTile::Type::GROUND_BOTTOM;

		// ceiling-ish platform
		for (int x = 5; x <= 18; ++x)
			room3.tiles[8][x] = MapTile::Type::GROUND_SURFACE;

		// one spike trap
		room3.trapCount = 1;
		room3.traps[0].type = (int)Trap::Type::SpikePlate;
		room3.traps[0].pos = { 10.5f, 1.5f };
		room3.traps[0].size = { 1.0f, 1.0f };
		room3.traps[0].upTime = 1.0f;
		room3.traps[0].downTime = 1.0f;
		room3.traps[0].damageOnHit = 10;
		room3.traps[0].startDisabled = false;

		roomMgr.SetRoom(ROOM_1, room1);
		roomMgr.SetRoom(ROOM_2, room2);
		roomMgr.SetRoom(ROOM_3, room3);
	}
}

void GameScene::ClearRuntimeRoomObjects()
{
	// rebuild managers by assignment/reset so we do not disturb other scene systems
	trapMgr = TrapManager{};

	enemyMgr = EnemyManager{};
	enemyMgr.SetBoss(&enemyBoss);
}

void GameScene::BuildCurrentRoom(RoomDirection cameFrom)
{
	if (roomMgr.GetCurrentRoomID() == ROOM_NONE)
		return;

	const RoomData& room = roomMgr.GetCurrentRoom();

	// clear current map
	for (int y = 0; y < ROOM_ROWS; ++y)
	{
		for (int x = 0; x < ROOM_COLS; ++x)
		{
			map.SetTile(x, y, MapTile::Type::NONE);
		}
	}

	// apply room tiles into active map
	for (int y = 0; y < ROOM_ROWS; ++y)
	{
		for (int x = 0; x < ROOM_COLS; ++x)
		{
			map.SetTile(x, y, room.tiles[y][x]);
		}
	}

	// clear and rebuild runtime room objects
	ClearRuntimeRoomObjects();

	// rebuild traps
	for (int i = 0; i < room.trapCount; ++i)
	{
		const RoomTrapSpawn& td = room.traps[i];

		Box box{};
		box.size = td.size;
		box.position = AEVec2{
			td.pos.x - td.size.x * 0.5f,
			td.pos.y - td.size.y * 0.5f
		};

		const Trap::Type tt = static_cast<Trap::Type>(td.type);

		if (tt == Trap::Type::SpikePlate)
		{
			trapMgr.Spawn<SpikePlate>(box, td.upTime, td.downTime, td.damageOnHit, td.startDisabled);
		}
		else if (tt == Trap::Type::PressurePlate)
		{
			trapMgr.Spawn<PressurePlate>(box);
		}
		else if (tt == Trap::Type::LavaPool)
		{
			trapMgr.Spawn<LavaPool>(box, td.damagePerTick, td.tickInterval);
		}
	}

	// rebuild enemies
	std::vector<EnemyManager::SpawnInfo> spawns;
	for (int i = 0; i < room.enemyCount; ++i)
	{
		spawns.push_back({
			(EnemySpawnType)room.enemies[i].preset,
			room.enemies[i].pos
			});
	}

	enemyMgr.SetBoss(&enemyBoss);
	enemyMgr.SetSpawns(spawns);
	enemyMgr.SpawnAll();

	// spawn player
	AEVec2 spawn = room.startSpawn;
	if (cameFrom != DIR_NONE)
		spawn = roomMgr.GetEntrySpawn(room.id, cameFrom);

	player.Reset(spawn);

	// fixed room camera (Celeste-style)
	camera.position = { ROOM_COLS * 0.5f, ROOM_ROWS * 0.5f };
	Camera::position = camera.position;
	AEGfxSetCamPosition(camera.position.x * Camera::scale, camera.position.y * Camera::scale);
}

RoomDirection GameScene::CheckRoomExit() const
{
	const AEVec2 p = player.GetPosition();

	if (p.y < 0.0f)
		return DIR_BOTTOM;
	if (p.y >= (float)ROOM_ROWS)
		return DIR_TOP;
	if (p.x < 0.0f)
		return DIR_LEFT;
	if (p.x >= (float)ROOM_COLS)
		return DIR_RIGHT;

	return DIR_NONE;
}

GameScene::GameScene() :
	map(ROOM_COLS, ROOM_ROWS),
	player(&map, &enemyMgr),
	camera({ 1,1 }, { (float)(ROOM_COLS - 1), (float)(ROOM_ROWS - 1) }, 64),
	testParticleSystem(
		20,
		ParticleSystem::EmitterSettings{
			.angleRange{ PI / 3, PI / 4 },
			.speedRange{ 30.f, 50.f },
			.lifetimeRange{1.f, 2.f},
		}
		),
	enemyBoss(35, 2.90f)
{
	// camera.SetFollow(&player.GetPosition(), 0, (float)ROOM_COLS, true);
	// =============================== Traps Setup (debug) ===========================
	//auto& plate = trapMgr.Spawn<PressurePlate>(
	//	Box{ {2.f, 4.f}, {4.f, 1.f} }   
	//);

	//auto& spikes = trapMgr.Spawn<SpikePlate>(
	//	Box{ {6.5f, 4.f}, {3.f, 1.f} }, 
	//	1.f, 1.f, 10, true
	//);
	// 
	//plate.AddLinkedTrap(&spikes);

	//trapMgr.Spawn<LavaPool>(
	//	Box{ {2.f, 3.0f}, {4.f, 0.8f} }, 
	//	2, 0.2f
	//);


	UI::Init(&player);
	Background::Init();
	// Init pause overlay resources 
	pauseRectMesh = MeshGenerator::GetRectMesh(1.0f, 1.0f);
	pauseCardBackTex = AEGfxTextureLoad("Assets/0_CardBack.png");

	// Load buff icon textures for pause overlay (same assets as BuffCardScreen)
	for (int i = 0; i < kPauseBuffTexCount; ++i) pauseBuffTex[i] = nullptr;

	// NOTE: These indices assume CARD_TYPE enum values are 0..N in this order.
	// If your enum order differs, adjust the mapping here.
	pauseBuffTex[(int)HERMES_FAVOR] = AEGfxTextureLoad("Assets/Hermes_Favor.png");
	pauseBuffTex[(int)IRON_DEFENCE] = AEGfxTextureLoad("Assets/Iron_Defence.png");
	pauseBuffTex[(int)SWITCH_IT_UP] = AEGfxTextureLoad("Assets/Switch_It_Up.png");
	pauseBuffTex[(int)REVITALIZE] = AEGfxTextureLoad("Assets/Revitalize.png");
	pauseBuffTex[(int)SHARPEN] = AEGfxTextureLoad("Assets/Sharpen.png");
	pauseBuffTex[(int)BERSERKER] = AEGfxTextureLoad("Assets/Berserker.png");
	pauseBuffTex[(int)FEATHERWEIGHT] = AEGfxTextureLoad("Assets/Featherweight.png");

	// Fonts for pause overlay
	pauseFontLarge = AEGfxCreateFont("Assets/m04.ttf", 55);
	pauseFontSmall = AEGfxCreateFont("Assets/m04.ttf", 35);
	pauseFontRuntime = AEGfxCreateFont("Assets/m04.ttf", 28);

	// Glow / emission textures (same as BuffCardScreen)
	for (int i = 0; i < kPauseRarityTexCount; ++i) pauseRarityTex[i] = nullptr;
	pauseRarityTex[RARITY_UNCOMMON] = AEGfxTextureLoad("Assets/Uncommon_Emission.png");
	pauseRarityTex[RARITY_RARE] = AEGfxTextureLoad("Assets/Rare_Emission.png");
	pauseRarityTex[RARITY_EPIC] = AEGfxTextureLoad("Assets/Epic_Emission.png");
	pauseRarityTex[RARITY_LEGENDARY] = AEGfxTextureLoad("Assets/Legendary_Emission.png");

	// Pixellari for description (match BuffCardScreen)
	pauseFontDesc = AEGfxCreateFont("Assets/Pixellari.ttf", 30); // change to your description font
}

GameScene::~GameScene()
{
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
	if (pauseFontRuntime)
	{
		AEGfxDestroyFont(pauseFontRuntime);
	}
	if (pauseRectMesh)
	{
		AEGfxMeshFree(pauseRectMesh);
		pauseRectMesh = nullptr;
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
}

void GameScene::Init()
{
	BuildTestRooms(roomMgr);
	roomMgr.SetCurrentRoom(ROOM_1);
	BuildCurrentRoom();
	roomTransitionLocked = false;
}

void GameScene::Update()
{
	// Toggle pause with ESC (GameScene only)
	if (AEInputCheckTriggered(AEVK_ESCAPE))
	{
		// If we are inside sub-pages, ESC returns to menu instead of unpausing
		if (pausePage == PausePage::Settings || pausePage == PausePage::ConfirmQuit)
			pausePage = PausePage::Menu;
		else
			TogglePause();
	}

	// When paused, skip gameplay update and only handle pause input
	if (IsPaused())
	{
		UpdatePauseInput();
		return;
	}

	float dt = static_cast<float>(Time::GetInstance().GetScaledDeltaTime());

	player.Update();

	// unlock only when player is back inside room bounds
	if (roomTransitionLocked)
	{
		if (CheckRoomExit() == DIR_NONE)
			roomTransitionLocked = false;
	}

	if (!roomTransitionLocked)
	{
		RoomDirection exitDir = CheckRoomExit();
		if (exitDir != DIR_NONE)
		{
			if (roomMgr.ChangeRoom(exitDir))
			{
				RoomDirection cameFrom = DIR_NONE;

				switch (exitDir)
				{
				case DIR_TOP:    cameFrom = DIR_BOTTOM; break;
				case DIR_LEFT:   cameFrom = DIR_RIGHT;  break;
				case DIR_BOTTOM: cameFrom = DIR_TOP;    break;
				case DIR_RIGHT:  cameFrom = DIR_LEFT;   break;
				default: break;
				}

				BuildCurrentRoom(cameFrom);
				roomTransitionLocked = true;
				return;
			}
		}
	}

	AEVec2 p = player.GetPosition();
	enemyMgr.UpdateAll(p, player.IsFacingRight(), map);
	//enemyBoss.Update(p, player.IsFacingRight());

	const AEVec2 pPos = player.GetPosition();
	const AEVec2 pSize = player.GetStats().playerSize;


	attackSystem.ApplyEnemyAttacksToPlayer(player, enemyMgr, &enemyBoss);

	// --- DELETE THIS LATER, PREVIOUS ENEMY ATTACK PLAYER!!!!! -------
	//const AEVec2 pPos = player.GetPosition();
	//const AEVec2 pSize = player.GetStats().playerSize;
	/*
	// Boss normal melee attack (ATTACK state via EnemyAttack)
	if (enemyBoss.PollAttackHit())
	{
		// EnemyAttack already checked absDx <= hitRange at hit moment (because Boss uses absDx in attack.Update).
		// Add a Y tolerance so it doesn't hit through platforms.
		const float dy = std::fabs(pPos.y - enemyBoss.position.y);
		const float yTol = (pSize.y * 0.5f) + 0.6f; // tune 0.3~1.0 depending on your level scale

		if (dy <= yTol)
		{
			player.TakeDamage(enemyBoss.attackDamage, enemyBoss.position); // choose your boss damage
			std::cout << "[Boss] HIT player (melee)\n";
		}
	}

	// Boss special spell damage
	const int spellHits = enemyBoss.ConsumeSpecialHits(player.GetPosition(),
	player.GetStats().playerSize);
	if (spellHits > 0)
	{
		const int spellDmg = 1;                 // tune
		player.TakeDamage(spellHits * spellDmg, enemyBoss.position);
		std::cout << "[Boss] spell hit x" << spellHits << "\n";
	}

	enemyMgr.ForEachEnemy([&](Enemy& e)
		{
			// do player/enemy AABB checks here
			// if hit: e.ApplyDamage(1);

			if (!e.PollAttackHit()) return;

			const AEVec2 ePos = e.GetPosition();

			const float dx = std::fabs(pPos.x - ePos.x);
			const float dy = std::fabs(pPos.y - ePos.y);

			// mid/close range on X, plus a small Y tolerance so it doesn't hit through floors
			if (dx <= e.GetAttackHitRange() && dy <= (pSize.y * 0.5f + 0.6f))
			{
				player.TakeDamage(e.GetAttackDamage(), e.GetPosition());
				std::cout << "Enemy HIT player!\n";
			}
		});
	*/

	trapMgr.Update(dt, player);

	//std::cout << std::fixed << std::setprecision(2) << AEFrameRateControllerGetFrameRate() << std::endl;

	// === Particle system test ===
	testParticleSystem.SetSpawnRate(AEInputCheckCurr(AEVK_F) ? 2000.f : 0.f);
	//testParticleSystem.SetSpawnRate(AEInputCheckCurr(AEVK_F) ? 5000000.f : 0.f);
	if (AEInputCheckTriggered(AEVK_G))
		testParticleSystem.SpawnParticleBurst(300);
	testParticleSystem.Update();

	// For checking current buffs vector
	//if (AEInputCheckTriggered(AEVK_P)) {
	//	std::cout << "Current buffs :\n";
	//	for (auto& buffs : BuffCardManager::GetCurrentBuffs()) {
	//		std::cout << "Buff: " << buffs.cardName << " Type: " << BuffCardManager::CardTypeToString(buffs.type) << " Rarity: "
	//			<< BuffCardManager::CardRarityToString(buffs.rarity) << "\nDescription: " << buffs.cardDesc << "\nEffect: " << buffs.cardEffect
	//			<< " Effect value 1: " << buffs.effectValue1 << " Effect value 2:" << buffs.effectValue2 << std::endl;
	//	}
	//}
	// === For Damage Text Testing ===
	//if AEInputCheckCurr/Triggered
	//if (AEInputCheckTriggered(AEVK_K))
	//{
	//	AEVec2 pos{};
	//	pos.x = AEExtras::RandomRange({ 2.5f, 24.f });
	//	pos.y = AEExtras::RandomRange({ 2.5f, 10.f });
	//	DAMAGE_TYPE type = static_cast<DAMAGE_TYPE>(AEExtras::RandomRange({ 0,6 }));
	//	int dmg = static_cast<int>(AEExtras::RandomRange({ 1,1000 }));
	//	UI::GetDamageTextSpawner().SpawnDamageText(dmg, type, pos);
	//}
	UI::GetDamageTextSpawner().Update();
	UI::Update();
	if (UI::GetRestartStatus()) { // Allow restart run from game over screen (i.e. load game scene again).
		UI::GetRestartStatus() = false;
		pausePage = PausePage::None;

		Time::GetInstance().ResetElapsedTime();
		TimerSystem::GetInstance().Clear();
		UI::Reset();
		if (!BuffCardManager::GetCurrentBuffs().empty()) {
			BuffCardManager::ResetCurrentBuffs(); // Only clears vector of held buffs.
		}
		// Reload the last successfully loaded level (if any)
		if (!gLastLoadedLevelPath.empty())
		{
			gPendingLevelPath = gLastLoadedLevelPath;
		}
		GSM::ChangeScene(SceneState::GS_GAME);
	}
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
	//trapMgr.Render();   // for debug
	testParticleSystem.Render();
	player.Render();
	enemyBoss.Render();
	enemyMgr.RenderAll();
	UI::Render();
	// Draw pause overlay on top of game render
	if (IsPaused())
	{
		RenderPauseOverlay();
	}

	// === Code below is for DEBUG ONLY ===

	// === Debug Info ===
	//Box playerBox{ player.GetPosition(), player.GetSize() };

	//Box plateBox{ {2.f, 4.f}, {4.f, 1.f} };
	//Box spikeBox{ {6.5f, 4.f}, {3.f, 1.f} };
	//Box lavaBox{ {2.f, 3.0f}, {4.f, 0.8f} };


	//bool onPlate = IntersectsBox(playerBox, plateBox);
	//bool onSpike = IntersectsBox(playerBox, spikeBox);
	//bool onLava = IntersectsBox(playerBox, lavaBox);

	//std::string ov = "Overlap: Plate=" + std::to_string(onPlate)
	//	+ " Spike=" + std::to_string(onSpike)
	//	+ " Lava=" + std::to_string(onLava);
	//QuickGraphics::PrintText(ov.c_str(), -1, 0.60f, 0.3f, 0.5f, 0.5f, 0.5f, 1);



#if _DEBUG
	AEVec2 worldMousePos;
	AEExtras::GetCursorWorldPosition(worldMousePos);
	std::string str = "World Mouse Pos:" + std::to_string(worldMousePos.x) + ", " + std::to_string(worldMousePos.y);
	QuickGraphics::PrintText(str.c_str(), -1, 0.95f, 0.3f, 0.5f, 0.5f, 0.5f, 1);
	str = "FPS:" + std::to_string(AEFrameRateControllerGetFrameRate());
	QuickGraphics::PrintText(str.c_str(), -1, 0.90f, 0.3f, 0.5f, 0.5f, 0.5f, 1);


	str = "Time:" + std::to_string(Time::GetInstance().GetScaledElapsedTime());
	QuickGraphics::PrintText(str.c_str(), -1, 0.85f, 0.3f, 0.5f, 0.5f, 0.5f, 1);

	std::string ppos = "Player Pos: " + std::to_string(player.GetPosition().x) + ", " + std::to_string(player.GetPosition().y);
	QuickGraphics::PrintText(ppos.c_str(), -1, 0.80f, 0.3f, 0.5f, 0.5f, 0.5f, 1);

	if (AEInputCheckTriggered(AEVK_R)) {
		pausePage = PausePage::None;

		Time::GetInstance().ResetElapsedTime();
		TimerSystem::GetInstance().Clear();
		UI::Reset();
		if (!BuffCardManager::GetCurrentBuffs().empty()) {
			BuffCardManager::ResetCurrentBuffs(); // Only clears vector of held buffs.
		}
		// Reload the last successfully loaded level (if any)
		if (!gLastLoadedLevelPath.empty())
		{
			gPendingLevelPath = gLastLoadedLevelPath;
		}
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
	// Screen: (0,0) top-left. Engine: (0,0) center, +Y up.
	return AEVec2{ px - w * 0.5f, (h * 0.5f) - py };
}

void GameScene::DrawDimBackground(float alpha)
{
	if (alpha <= 0.0f) return;

	// Full-screen overlay in WORLD space, aligned to current camera (same as BuffCardScreen::DrawBlackOverlay)
	AEMtx33 scale, rotate, translate, transform;

	AEMtx33Scale(&scale,
		(float)AEGfxGetWindowWidth() * 2.0f,
		(float)AEGfxGetWindowHeight() * 2.0f);

	AEMtx33Rot(&rotate, 0.0f);

	// IMPORTANT: follow camera so it covers the viewport regardless of camera movement
	AEMtx33Trans(&translate,
		Camera::position.x * Camera::scale,
		Camera::position.y * Camera::scale);

	AEMtx33Concat(&transform, &rotate, &scale);
	AEMtx33Concat(&transform, &translate, &transform);

	AEGfxSetRenderMode(AE_GFX_RM_COLOR);

	// Solid black with alpha
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

	// Use the same render-state pattern as BuffCardScreen::DrawBlackOverlay()
	AEGfxSetRenderMode(AE_GFX_RM_COLOR);

	// Multiply should stay neutral; darkening comes from the additive alpha
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

	// Convert pixel position to NDC position for AEGfxPrint
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
	double t = Time::GetInstance().GetScaledElapsedTime(); // seconds
	int totalMs = (int)(t * 1000.0);

	int mm = (totalMs / 60000) % 100;      // minutes
	int ss = (totalMs / 1000) % 60;        // seconds
	int cs = (totalMs / 10) % 100;         // centiseconds (00-99)

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
			TogglePause();
			return;
		}
		if (IsClicked(btnRestart))
		{
			pausePage = PausePage::None;
			Time::GetInstance().SetPaused(false);

			Time::GetInstance().ResetElapsedTime();
			TimerSystem::GetInstance().Clear();
			UI::Reset();
			if (!BuffCardManager::GetCurrentBuffs().empty()) {
				BuffCardManager::ResetCurrentBuffs(); // Only clears vector of held buffs.
			}
			// Reload the last successfully loaded level (if any)
			if (!gLastLoadedLevelPath.empty())
			{
				gPendingLevelPath = gLastLoadedLevelPath;
			}
			GSM::ChangeScene(SceneState::GS_GAME);
			return;
		}
		if (IsClicked(btnSettings))
		{
			pausePage = PausePage::Settings;
			return;
		}
		if (IsClicked(btnMenu))
		{
			pausePage = PausePage::ConfirmQuit;
			return;
		}
	}
	else if (pausePage == PausePage::ConfirmQuit)
	{
		float w = (float)AEGfxGetWindowWidth();
		float h = (float)AEGfxGetWindowHeight();

		UIRect btnNo{ { w * 0.5f - 90, h * 0.5f + 30 }, {140, 44} };
		UIRect btnYes{ { w * 0.5f + 90, h * 0.5f + 30 }, {140, 44} };

		if (IsClicked(btnNo))
		{
			pausePage = PausePage::Menu;
			return;
		}
		if (IsClicked(btnYes))
		{
			pausePage = PausePage::None;
			Time::GetInstance().SetPaused(false);
			GSM::ChangeScene(SceneState::GS_MAIN_MENU);
			return;
		}
	}
	else if (pausePage == PausePage::Settings)
	{
		// Placeholder: you can add sliders later; for now just stay here
	}
}

void GameScene::RenderPauseOverlay()
{
	float w = (float)AEGfxGetWindowWidth();
	float h = (float)AEGfxGetWindowHeight();

	// Dim background
	DrawDimBackground(0.75f);


	// Titles
	DrawTextPx(pauseFontLarge, "PAUSED", 40, 100, 1.0f, 1, 1, 1, 1);
	DrawTextPx(pauseFontRuntime, "Run Time : " + FormatRunTime(), 40, 150, 1.0f, 1, 1, 1, 1);

	// Buttons
	auto drawBtn = [&](const char* label, const UIRect& r)
		{
			const bool hover = IsMouseOver(r);

			// 1) add a subtle scaling effect when hover (like it's "lifting up" towards the player)
			const float textScale = hover ? 1.05f : 1.0f;

			// 2) change text color when hover; also make non-hovered buttons slightly transparent to de-emphasize them (except the hovered one, which is fully opaque)
			float cr = 1.f, cg = 1.f, cb = 1.f, ca = hover ? 1.f : 0.85f;
			if (hover)
			{
				cr = 1.0f;  cg = 0.95f; cb = 0.35f; // bright yellow for hover
			}

			// 3) text only
			DrawTextPx(
				pauseFontSmall,
				label,
				r.pos.x - 55,
				r.pos.y + 6,
				textScale,
				cr, cg, cb, ca
			);
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
		DrawTextPx(pauseFontLarge, "SETTINGS", 40, 160, 1.0f, 1, 1, 1, 1);
		DrawTextPx(pauseFontSmall, "Press ESC to go back.", 40, 200, 1.0f, 1, 1, 1, 1);
	}

	// ============================== Active Buffs (top-right) ==============================
	// Draw only existing buffs. 1 row max, 4 cards per row. No placeholders.
	const auto& buffs = BuffCardManager::GetCurrentBuffs();
	if (!buffs.empty())
	{

		const int cols = 3;
		const int maxRows = 3;
		const int maxCards = cols * maxRows; // 9

		// Slightly smaller cards
		const float cardW = 150.0f;
		const float cardH = 212.0f;
		const float gapX = 50.0f;
		const float gapY = 30.0f;

		const int count = (int)buffs.size();
		const int drawCount = (count < maxCards) ? count : maxCards;


		// Anchor: move this block to the right & top area (match your red mark)
		// (0,0) is top-left in pixel coordinates
		const float anchorX = w * 0.56f;   // increase => move right, decrease => move left
		const float anchorY = 150.0f;      // increase => move down, decrease => move up

		// Title position (aligned with cards)
		DrawTextPx(pauseFontLarge, "ACTIVE BUFFS:", anchorX, 100.0f, 0.95f, 1, 1, 1, 1);



		for (int i = 0; i < drawCount; ++i)
		{
			const BuffCard& b = buffs[i];

			UIRect card;
			card.size = { cardW, cardH };

			// UIRect.pos is center-based (pixel coords)
			const int cx = i % cols;   // column index: 0,1,2
			const int cy = i / cols;   // row index: 0,0,0,1,1,1,...

			const float centerX = anchorX + cx * (cardW + gapX) + cardW * 0.5f;
			const float centerY = anchorY + cy * (cardH + gapY) + cardH * 0.5f;
			card.pos = { centerX, centerY };

			// Pick buff front texture by card type; fallback to card back if missing
			AEGfxTexture* tex = nullptr;
			int typeIdx = (int)b.type;
			if (typeIdx >= 0 && typeIdx < kPauseBuffTexCount)
				tex = pauseBuffTex[typeIdx];
			if (!tex)
				tex = pauseCardBackTex;

			DrawTexturePanel(tex, card, 1.0f);

			// --- draw glow (rarity emission) ---
			AEGfxTexture* glow = nullptr;
			int r = (int)b.rarity;
			if (r >= 0 && r < kPauseRarityTexCount) glow = pauseRarityTex[r];

			if (glow)
			{
				const float EMISSION_SCALE = 1.15f; // same as BuffCardScreen 
				UIRect glowRect = card;
				glowRect.size.x *= EMISSION_SCALE;
				glowRect.size.y *= EMISSION_SCALE;
				// pos same as card center, so glow is centered on card
				DrawTexturePanel(glow, glowRect, 1.0f);
			}

			// Hover tooltip (text only)
			if (IsMouseOver(card))
			{
				// tooltip panel
				DrawSolidPanel(UIRect{ { w * 0.5f, h - 90.0f }, { w * 0.85f, 90.0f } }, 0.55f);

				// Title (m04)
				DrawTextPx(
					pauseFontSmall,
					b.cardName,
					120.0f, h - 125.0f, 1.0f,
					1, 1, 1, 1
				);

				// Description/effect (Pixellari) - using cardEffect if available, otherwise fallback to cardDesc. 
				const std::string desc = b.cardEffect.empty() ? b.cardDesc : b.cardEffect;

				DrawTextPx(
					pauseFontDesc,
					desc,
					120.0f, h - 95.0f, 1.0f,
					0.9f, 0.9f, 0.9f, 1.0f
				);
			}
		}
	}
	// ======================================================================================
}