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
#include <cmath>
#include "../AudioManager.h"

std::string gPendingLevelPath;   // defined here, extern'd in MainMenuScene.cpp
std::string gLastLoadedLevelPath; // last successfully loaded level path for restart

namespace
{
	// squared-distance check (no sqrt)
	bool IsNear(const AEVec2& a, const AEVec2& b, float range)
	{
		float dx = b.x - a.x;
		float dy = b.y - a.y;
		return (dx * dx + dy * dy) <= (range * range);
	}

	RoomID RoomIdFromIndex(int idx)
	{
		if (idx < 0 || idx >= ROOM_COUNT)
			return ROOM_NONE;
		return static_cast<RoomID>(idx);
	}

	bool PointInRoom(const AEVec2& p, int roomX, int roomY)
	{
		const float minX = (float)(roomX * ROOM_COLS);
		const float minY = (float)(roomY * ROOM_ROWS);
		const float maxX = minX + (float)ROOM_COLS;
		const float maxY = minY + (float)ROOM_ROWS;

		return p.x >= minX && p.x < maxX &&
			p.y >= minY && p.y < maxY;
	}

	void BuildRoomsFromLevelData(const LevelData& lvl, RoomManager& roomMgr, RoomID& outStartRoom)
	{
		roomMgr.Clear();
		outStartRoom = ROOM_1;

		if (lvl.cols <= 0 || lvl.rows <= 0)
			return;

		const int roomsX = lvl.cols / ROOM_COLS;
		const int roomsY = lvl.rows / ROOM_ROWS;



		if (roomsX <= 0 || roomsY <= 0)
			return;

		const int totalRooms = roomsX * roomsY;
		const int roomCountToUse = (totalRooms < ROOM_COUNT) ? totalRooms : ROOM_COUNT;

		for (int i = 0; i < roomCountToUse; ++i)
		{
			const int rx = i % roomsX;
			const int ry = i / roomsX;

			RoomData room{};
			room.id = RoomIdFromIndex(i);
			std::cout << "lvl.cols=" << lvl.cols
				<< " lvl.rows=" << lvl.rows
				<< " roomsX=" << roomsX
				<< " roomsY=" << roomsY
				<< " totalRooms=" << (roomsX * roomsY)
				<< "\n";

			// neighbors
			if (ry - 1 >= 0)
				room.topRoom = RoomIdFromIndex(i - roomsX);
			if (ry + 1 < roomsY && i + roomsX < roomCountToUse)
				room.bottomRoom = RoomIdFromIndex(i + roomsX);
			if (rx - 1 >= 0)
				room.leftRoom = RoomIdFromIndex(i - 1);
			if (rx + 1 < roomsX && i + 1 < roomCountToUse)
				room.rightRoom = RoomIdFromIndex(i + 1);

			// tiles
			for (int y = 0; y < ROOM_ROWS; ++y)
			{
				for (int x = 0; x < ROOM_COLS; ++x)
				{
					const int globalX = rx * ROOM_COLS + x;
					const int globalY = ry * ROOM_ROWS + y;

					int v = (int)MapTile::Type::NONE;
					if (globalX >= 0 && globalX < lvl.cols &&
						globalY >= 0 && globalY < lvl.rows)
					{
						v = lvl.tiles[(size_t)globalY * (size_t)lvl.cols + (size_t)globalX];
					}

					if (v < 0 || v >= MapTile::typeCount)
						v = (int)MapTile::Type::NONE;

					room.tiles[y][x] = (MapTile::Type)v;
				}
			}

			// default entry points
			room.entryFromLeft = { 3.0f, 3.0f };
			room.entryFromRight = { (float)ROOM_COLS - 4.0f, 3.0f };
			room.entryFromTop = { ROOM_COLS * 0.5f, (float)ROOM_ROWS - 4.0f };
			room.entryFromBottom = { ROOM_COLS * 0.5f, 3.0f };

			// default start spawn
			room.startSpawn = { 2.5f, 3.0f };

			// spawn belongs to this room?
			if (PointInRoom(lvl.spawn, rx, ry))
			{
				room.startSpawn = {
					lvl.spawn.x - rx * (float)ROOM_COLS,
					lvl.spawn.y - ry * (float)ROOM_ROWS
				};
				outStartRoom = room.id;
			}

			// enemies in this room
			for (const auto& e : lvl.enemies)
			{
				if (!PointInRoom(e.pos, rx, ry))
					continue;

				if (room.enemyCount >= MAX_ROOM_ENEMIES)
					continue;

				room.enemies[room.enemyCount].preset = e.preset;
				room.enemies[room.enemyCount].pos = {
					e.pos.x - rx * (float)ROOM_COLS,
					e.pos.y - ry * (float)ROOM_ROWS
				};
				++room.enemyCount;
			}

			// traps in this room
			for (const auto& t : lvl.traps)
			{
				if (!PointInRoom(t.pos, rx, ry))
					continue;

				if (room.trapCount >= MAX_ROOM_TRAPS)
					continue;

				room.traps[room.trapCount].type = t.type;
				room.traps[room.trapCount].pos = {
					t.pos.x - rx * (float)ROOM_COLS,
					t.pos.y - ry * (float)ROOM_ROWS
				};
				room.traps[room.trapCount].size = t.size;
				room.traps[room.trapCount].upTime = t.upTime;
				room.traps[room.trapCount].downTime = t.downTime;
				room.traps[room.trapCount].damageOnHit = t.damageOnHit;
				room.traps[room.trapCount].startDisabled = t.startDisabled;
				room.traps[room.trapCount].damagePerTick = t.damagePerTick;
				room.traps[room.trapCount].tickInterval = t.tickInterval;
				++room.trapCount;
			}

			roomMgr.SetRoom(room.id, room);
			std::cout << "room " << (int)room.id
				<< " rx=" << rx << " ry=" << ry
				<< " L=" << (int)room.leftRoom
				<< " R=" << (int)room.rightRoom
				<< " T=" << (int)room.topRoom
				<< " B=" << (int)room.bottomRoom
				<< "\n";
		}
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
	UI::Init(&player);
	Background::Init();
	// Init pause overlay resources 
	pauseRectMesh = MeshGenerator::GetRectMesh(1.0f, 1.0f);
	pauseCardBackTex = AEGfxTextureLoad("Assets/0_CardBack.png");

	// Load buff icon textures for pause overlay (same assets as BuffCardScreen)
	for (int i = 0; i < kPauseBuffTexCount; ++i) pauseBuffTex[i] = nullptr;

	// NOTE: These indices assume CARD_TYPE enum values are 0..N in this order.
	pauseBuffTex[(int)HERMES_FAVOR] = AEGfxTextureLoad("Assets/Hermes_Favor.png");
	pauseBuffTex[(int)IRON_DEFENCE] = AEGfxTextureLoad("Assets/Iron_Defence.png");
	pauseBuffTex[(int)SWITCH_IT_UP] = AEGfxTextureLoad("Assets/Switch_It_Up.png");
	pauseBuffTex[(int)REVITALIZE] = AEGfxTextureLoad("Assets/Revitalize.png");
	pauseBuffTex[(int)SHARPEN] = AEGfxTextureLoad("Assets/Sharpen.png");
	pauseBuffTex[(int)BERSERKER] = AEGfxTextureLoad("Assets/Berserker.png");
	pauseBuffTex[(int)FLEETING_STEP] = AEGfxTextureLoad("Assets/Fleeting_Step.png");
	pauseBuffTex[(int)SUREFOOTED] = AEGfxTextureLoad("Assets/Surefooted.png");
	pauseBuffTex[(int)DEEP_VITALITY] = AEGfxTextureLoad("Assets/Deep_Vitality.png");
	pauseBuffTex[(int)HAND_OF_FATE] = AEGfxTextureLoad("Assets/Hand_Of_Fate.png");
	pauseBuffTex[(int)SUNDERING_BLOW] = AEGfxTextureLoad("Assets/Sundering_Blow.png");

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
	pauseFontDesc = AEGfxCreateFont("Assets/Pixellari.ttf", 30);
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
	roomMgr.Clear();

	bool loadedFromFile = false;
	LevelData loadedLevel{};
	RoomID startRoom = ROOM_1;

	if (!gPendingLevelPath.empty())
	{
		std::cout << "pending path: " << gPendingLevelPath << "\n";

		LevelData lvl;
		if (LoadLevelFromFile(gPendingLevelPath.c_str(), lvl))
		{
			std::cout << "load success\n";
			std::cout << "loaded rows=" << lvl.rows << " cols=" << lvl.cols << "\n";

			loadedFromFile = true;
			loadedLevel = lvl;
			gLastLoadedLevelPath = gPendingLevelPath;
			BuildRoomsFromLevelData(loadedLevel, roomMgr, startRoom);
			gPendingLevelPath.clear();
		}
		else
		{
			std::cout << "load failed\n";
		}

		gPendingLevelPath.clear();
	}
	else
	{
		std::cout << "pending path empty\n";
	}

	if (!loadedFromFile)
	{
		// fallback: build a single empty-ish room if no file was loaded
		LevelData lvl{};
		lvl.rows = ROOM_ROWS;
		lvl.cols = ROOM_COLS;
		lvl.spawn = { 2.5f, 3.0f };
		lvl.tiles.assign((size_t)ROOM_ROWS * (size_t)ROOM_COLS, (int)MapTile::Type::NONE);

		for (int x = 0; x < ROOM_COLS; ++x)
			lvl.tiles[(size_t)0 * ROOM_COLS + x] = (int)MapTile::Type::GROUND_BOTTOM;

		BuildRoomsFromLevelData(lvl, roomMgr, startRoom);
	}

	roomMgr.SetCurrentRoom(startRoom);
	const RoomData& r = roomMgr.GetCurrentRoom();
	std::cout << "startRoom=" << (int)startRoom
		<< " L=" << (int)r.leftRoom
		<< " R=" << (int)r.rightRoom
		<< " T=" << (int)r.topRoom
		<< " B=" << (int)r.bottomRoom
		<< "\n";
	BuildCurrentRoom();
	roomTransitionLocked = false;
	AudioManager::PlayMusic(MusicId::GameScene, 1.0f, 1.0f, -1);
	std::cout << "lvl.cols=" << loadedLevel.cols
		<< " lvl.rows=" << loadedLevel.rows
		<< "\n";
}

void GameScene::Update()
{
	// Toggle pause with ESC (GameScene only)
	if (AEInputCheckTriggered(AEVK_ESCAPE))
	{
		// If we are inside sub-pages, ESC returns to menu instead of unpausing
		if (pausePage == PausePage::Settings || pausePage == PausePage::ConfirmQuit || pausePage == PausePage::ConfirmRestart)
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
			else
			{
				AEVec2 p = player.GetPosition();

				if (p.x < 0.0f) p.x = 0.0f;
				if (p.x > (float)ROOM_COLS - 0.1f) p.x = (float)ROOM_COLS - 0.1f;
				if (p.y < 0.0f) p.y = 0.0f;
				if (p.y > (float)ROOM_ROWS - 0.1f) p.y = (float)ROOM_ROWS - 0.1f;

				player.Reset(p);
			}
		}
	}

	AEVec2 p = player.GetPosition();
	enemyMgr.UpdateAll(p, player.GetIsFacingRight(), map);

	const AEVec2 pPos = player.GetPosition();
	const AEVec2 pSize = player.GetStats().playerSize;

	attackSystem.UpdateEnemyAttack(player, enemyMgr, &enemyBoss, map);

	trapMgr.Update(dt, player);

	testParticleSystem.SetSpawnRate(AEInputCheckCurr(AEVK_F) ? 2000.f : 0.f);
	if (AEInputCheckTriggered(AEVK_G))
		testParticleSystem.SpawnParticleBurst(300);
	testParticleSystem.Update();

	UI::GetDamageTextSpawner().Update();
	UI::Update();
	if (UI::GetRestartStatus()) { // Allow restart run from game over screen
		UI::GetRestartStatus() = false;
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
	testParticleSystem.Render();
	player.Render();
	enemyBoss.Render();
	enemyMgr.RenderAll();
	UI::Render();

	if (IsPaused())
	{
		RenderPauseOverlay();
	}

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
			BuffCardManager::ResetCurrentBuffs();
		}
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
			TogglePause();
			return;
		}
		if (IsClicked(btnRestart))
		{
			pausePage = PausePage::ConfirmRestart;
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

		UIRect btnNo{ { w * 0.5f - 100, h * 0.5f + 30 }, {140, 44} };
		UIRect btnYes{ { w * 0.5f + 100, h * 0.5f + 30 }, {140, 44} };

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
			pausePage = PausePage::Menu;
			return;
		}
		if (IsClicked(btnYes))
		{
			pausePage = PausePage::None;
			Time::GetInstance().SetPaused(false);
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

		UIRect bgmTrack = MakeTrackRect(bgmY);
		UIRect sfxTrack = MakeTrackRect(sfxY);
		UIRect bgmKnob = MakeKnobRect(AudioManager::GetMusicVolume(), bgmY);
		UIRect sfxKnob = MakeKnobRect(AudioManager::GetSFXVolume(), sfxY);

		if (mousePressed)
		{
			if (PointInRectPx((float)mx, (float)my, bgmTrack) || PointInRectPx((float)mx, (float)my, bgmKnob))
				draggingBgmSlider = true;

			if (PointInRectPx((float)mx, (float)my, sfxTrack) || PointInRectPx((float)mx, (float)my, sfxKnob))
				draggingSfxSlider = true;
		}

		if (!mouseHeld)
		{
			draggingBgmSlider = false;
			draggingSfxSlider = false;
		}

		if (draggingBgmSlider)
			AudioManager::SetMusicVolume(SliderValueFromMouse((float)mx, sliderLeft, sliderWidth));

		if (draggingSfxSlider)
			AudioManager::SetSFXVolume(SliderValueFromMouse((float)mx, sliderLeft, sliderWidth));
	}
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

		DrawSlider("BGM Volume", AudioManager::GetMusicVolume(), bgmY, draggingBgmSlider);
		DrawSlider("SFX Volume", AudioManager::GetSFXVolume(), sfxY, draggingSfxSlider);
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
}