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
#include "../Rooms/RoomBuilder.h"
#include "../enemy/AttackSystem.h"
#include <algorithm>

std::string gPendingLevelPath = "Assets/Levels/gamescene.lvl";   // defined here, extern'd in MainMenuScene.cpp
std::string gLastLoadedLevelPath; // last successfully loaded level path for restart

void GameScene::BuildCurrentRoom(RoomDirection cameFrom, const AEVec2* forcedSpawn)
{
	if (roomMgr.GetCurrentRoomID() == ROOM_NONE)
		return;

	const RoomData& room = roomMgr.GetCurrentRoom();
	const AEVec2 roomOrigin = GetRoomOrigin(room.id);

	// Do NOT rebuild the map tiles here anymore.
	// The full level map is built once in Init().

	ClearRuntimeRoomObjects();

	struct PendingPlateBinding
	{
		PressurePlate* plate = nullptr;
		const RoomTrapSpawn* def = nullptr;
	};

	std::vector<PendingPlateBinding> pendingPlates;
	std::vector<std::pair<int, Trap*>> spawnedById;
	std::vector<Trap*> spawnedSpikes;
	bool hasExplicitLinks = false;

	for (int i = 0; i < room.trapCount; ++i)
	{
		const RoomTrapSpawn& td = room.traps[i];

		Box box{};
		box.size = td.size;
		box.position = AEVec2{
			roomOrigin.x + td.pos.x - td.size.x * 0.5f,
			roomOrigin.y + td.pos.y - td.size.y * 0.5f
		};

		Trap* spawnedTrap = nullptr;
		const Trap::Type tt = static_cast<Trap::Type>(td.type);

		if (tt == Trap::Type::SpikePlate)
		{
			SpikePlate& spikeRef =
				trapMgr.Spawn<SpikePlate>(box, td.upTime, td.downTime, td.damageOnHit, td.startDisabled);

			spawnedTrap = &spikeRef;
			spawnedSpikes.push_back(&spikeRef);
		}
		else if (tt == Trap::Type::PressurePlate)
		{
			PressurePlate& plateRef = trapMgr.Spawn<PressurePlate>(box);
			spawnedTrap = &plateRef;

			pendingPlates.push_back({ &plateRef, &td });
			if (!td.links.empty())
				hasExplicitLinks = true;
		}
		else if (tt == Trap::Type::LavaPool)
		{
			LavaPool& lavaRef = trapMgr.Spawn<LavaPool>(box, td.damagePerTick, td.tickInterval);
			spawnedTrap = &lavaRef;
		}

		if (spawnedTrap && td.id >= 0)
			spawnedById.push_back({ td.id, spawnedTrap });
	}

	auto FindSpawnedTrapById = [&spawnedById](int id) -> Trap*
		{
			for (const auto& entry : spawnedById)
			{
				if (entry.first == id)
					return entry.second;
			}
			return nullptr;
		};

	if (hasExplicitLinks)
	{
		for (const auto& pending : pendingPlates)
		{
			if (!pending.plate || !pending.def)
				continue;

			for (int linkId : pending.def->links)
			{
				Trap* target = FindSpawnedTrapById(linkId);
				if (target)
					pending.plate->AddLinkedTrap(target);
			}
		}
	}
	else
	{
		for (const auto& pending : pendingPlates)
		{
			if (!pending.plate)
				continue;

			for (Trap* spike : spawnedSpikes)
			{
				if (spike)
					pending.plate->AddLinkedTrap(spike);
			}
		}
	}

	std::vector<EnemyManager::SpawnInfo> spawns;
	bool hasBoss = false;

	for (int i = 0; i < room.enemyCount; ++i)
	{
		EnemySpawnType type = (EnemySpawnType)room.enemies[i].preset;

		AEVec2 worldPos{
			roomOrigin.x + room.enemies[i].pos.x,
			roomOrigin.y + room.enemies[i].pos.y
		};

		spawns.push_back({
			type,
			worldPos
			});

		if (type == EnemySpawnType::Boss)
			hasBoss = true;
	}

	activeBoss = hasBoss ? &enemyBoss : nullptr;

	enemyMgr.SetBoss(activeBoss);
	enemyMgr.SetCurrentRoomID(room.id);
	enemyMgr.SetSpawns(spawns);
	enemyMgr.SpawnAll();


	AEVec2 spawn{
		roomOrigin.x + room.startSpawn.x,
		roomOrigin.y + room.startSpawn.y
	};

	if (forcedSpawn)
	{
		spawn = *forcedSpawn;
	}
	else if (cameFrom != DIR_NONE)
	{
		const AEVec2 roomMin{
			roomOrigin.x + 0.35f,
			roomOrigin.y + 0.35f
		};
		const AEVec2 roomMax{
			roomOrigin.x + static_cast<float>(ROOM_COLS) - 0.35f,
			roomOrigin.y + static_cast<float>(ROOM_ROWS) - 0.35f
		};

		spawn = AEVec2{
			std::clamp(spawn.x, roomMin.x, roomMax.x),
			std::clamp(spawn.y, roomMin.y, roomMax.y)
		};
	}

	/*if (cameFrom == DIR_NONE && forcedSpawn == nullptr)
		player.Reset(spawn);      // first load into GameScene
	else
		player.SetPosition(spawn); // room transition*/
	

	const bool snapCamera = (cameFrom == DIR_NONE);
	camera.SetFollow(&player.GetPosition(), 0.f, 0.f, snapCamera);
	if (snapCamera)
		camera.Update();

	ApplyBlockedReturnBarrier();

	if (roomMgr.GetCurrentRoomID() == ROOM_2)
		UI::StartBossIntro();
}

void GameScene::ClearRuntimeRoomObjects()
{
	// rebuild managers by assignment/reset so we do not disturb other scene systems
	trapMgr = TrapManager{};

	enemyMgr = EnemyManager{};
	activeBoss = nullptr;
	enemyMgr.SetBoss(nullptr);
}

int GameScene::GetRoomsPerRow() const
{
	return max(1, mapCols / ROOM_COLS);
}

AEVec2 GameScene::GetRoomOrigin(RoomID id) const
{
	if (id == ROOM_NONE || !roomMgr.HasRoom(id))
		return AEVec2{ 0.f, 0.f };

	const RoomData& room = roomMgr.GetRoom(id);

	return AEVec2{
		room.gridX * static_cast<float>(ROOM_COLS),
		room.gridY * static_cast<float>(ROOM_ROWS)
	};
}

AEVec2 GameScene::ComputeTransitionSpawn(
	RoomID previousRoom,
	RoomID nextRoom,
	const AEVec2& previousPos) const
{
	static constexpr float kInset = 0.35f;

	const AEVec2 prevOrigin = GetRoomOrigin(previousRoom);
	const AEVec2 nextOrigin = GetRoomOrigin(nextRoom);

	AEVec2 spawn = previousPos;

	auto ClampFloat = [](float v, float lo, float hi) -> float
		{
			return (v < lo) ? lo : ((v > hi) ? hi : v);
		};

	const float nextMinX = nextOrigin.x + kInset;
	const float nextMaxX = nextOrigin.x + static_cast<float>(ROOM_COLS) - kInset;
	const float nextMinY = nextOrigin.y + kInset;
	const float nextMaxY = nextOrigin.y + static_cast<float>(ROOM_ROWS) - kInset;

	if (nextOrigin.x > prevOrigin.x)
	{
		spawn.x = nextMinX;
		spawn.y = ClampFloat(previousPos.y, nextMinY, nextMaxY);
	}
	else if (nextOrigin.x < prevOrigin.x)
	{
		spawn.x = nextMaxX;
		spawn.y = ClampFloat(previousPos.y, nextMinY, nextMaxY);
	}
	else if (nextOrigin.y > prevOrigin.y)
	{
		spawn.x = ClampFloat(previousPos.x, nextMinX, nextMaxX);
		spawn.y = nextMinY;
	}
	else if (nextOrigin.y < prevOrigin.y)
	{
		spawn.x = ClampFloat(previousPos.x, nextMinX, nextMaxX);
		spawn.y = nextMaxY;
	}
	else
	{
		spawn.x = ClampFloat(previousPos.x, nextMinX, nextMaxX);
		spawn.y = ClampFloat(previousPos.y, nextMinY, nextMaxY);
	}

	return spawn;
}


RoomDirection GameScene::CheckRoomExit() const
{
	if (roomMgr.GetCurrentRoomID() == ROOM_NONE)
		return DIR_NONE;

	const AEVec2 p = player.GetPosition();
	const AEVec2 origin = GetRoomOrigin(roomMgr.GetCurrentRoomID());

	if (p.y < origin.y)
		return DIR_BOTTOM;
	if (p.y >= origin.y + static_cast<float>(ROOM_ROWS))
		return DIR_TOP;
	if (p.x < origin.x)
		return DIR_LEFT;
	if (p.x >= origin.x + static_cast<float>(ROOM_COLS))
		return DIR_RIGHT;

	return DIR_NONE;
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

void GameScene::ApplyBlockedReturnBarrier()
{
	if (blockedReturnDir == DIR_NONE || roomMgr.GetCurrentRoomID() == ROOM_NONE)
		return;

	const AEVec2 origin = GetRoomOrigin(roomMgr.GetCurrentRoomID());
	const int ox = static_cast<int>(origin.x);
	const int oy = static_cast<int>(origin.y);

	// use a tile type that is definitely solid in your collision
	const MapTile::Type kBlockTile = MapTile::Type::GROUND_BODY;

	switch (blockedReturnDir)
	{
	case DIR_BOTTOM:
		// solid floor row at the room's bottom edge
		for (int x = 0; x < ROOM_COLS; ++x)
			map.SetTile(ox + x, oy - 1, kBlockTile);
		break;

	case DIR_TOP:
		// solid ceiling row at the room's top edge
		for (int x = 0; x < ROOM_COLS; ++x)
			map.SetTile(ox + x, oy + 1 + ROOM_ROWS - 1, kBlockTile);
		break;

	case DIR_LEFT:
		// solid wall column at the room's left edge
		for (int y = 0; y < ROOM_ROWS; ++y)
			map.SetTile(ox - 1, oy + y, kBlockTile);
		break;

	case DIR_RIGHT:
		// solid wall column at the room's right edge
		for (int y = 0; y < ROOM_ROWS; ++y)
			map.SetTile(ox + 1 + ROOM_COLS - 1, oy + y, kBlockTile);
		break;

	default:
		break;
	}
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
	AudioManager::Exit();
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
		LevelData lvl{};
		lvl.rows = ROOM_ROWS;
		lvl.cols = ROOM_COLS;
		lvl.spawn = { 2.5f, 3.0f };
		lvl.tiles.assign((size_t)ROOM_ROWS * (size_t)ROOM_COLS, (int)MapTile::Type::NONE);

		for (int x = 0; x < ROOM_COLS; ++x)
			lvl.tiles[(size_t)0 * ROOM_COLS + x] = (int)MapTile::Type::GROUND_BOTTOM;

		loadedLevel = lvl;
		BuildRoomsFromLevelData(loadedLevel, roomMgr, startRoom);
	}

	mapCols = loadedLevel.cols;
	mapRows = loadedLevel.rows;

	// Rebuild full level map
	map.~MapGrid();
	new (&map) MapGrid(mapCols, mapRows);

	for (int y = 0; y < mapRows; ++y)
	{
		for (int x = 0; x < mapCols; ++x)
		{
			int v = loadedLevel.tiles[(size_t)y * (size_t)mapCols + (size_t)x];
			if (v < 0 || v >= MapTile::typeCount)
				v = (int)MapTile::Type::NONE;

			map.SetTile(x, y, (MapTile::Type)v);
		}
	}

	// Rebuild camera with full level bounds
	camera.~Camera();
	new (&camera) Camera(
		{ 0.f, 0.f },
		{ (float)mapCols, (float)mapRows },
		64.0f
	);

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
	blockedReturnDir = DIR_NONE;
	//AudioManager::PlayMusic(MusicId::GameScene, 1.0f, 1.0f, -1);
	std::cout << "lvl.cols=" << loadedLevel.cols
		<< " lvl.rows=" << loadedLevel.rows
		<< "\n";
	if (roomMgr.GetCurrentRoomID() == ROOM_1) {
		if (AudioManager::gameMusic)   // make sure the pointer is initialized
			AudioManager::gameMusic->Play(1.0f);  // pass volume
	}


	player.Reset({ 1, 7.5 });
}

void GameScene::Update()
{
	// Toggle pause with ESC (GameScene only)
	if (AEInputCheckTriggered(AEVK_ESCAPE))
	{
		// If we are inside sub-pages, ESC returns to menu instead of unpausing
		if (pausePage == PausePage::Settings || pausePage == PausePage::ConfirmQuit || pausePage == PausePage::ConfirmRestart) {
			pausePage = PausePage::Menu;
			AudioManager::UnmuffleGameMusic();
		}
		else
		{
			AudioManager::MuffleGameMusic();
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
			// Block going back through the side we just entered from
			if (exitDir == blockedReturnDir)
			{
				return;
			}

			const RoomID previousRoom = roomMgr.GetCurrentRoomID();
			const AEVec2 previousPos = player.GetPosition();

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

				const RoomID nextRoom = roomMgr.GetCurrentRoomID();
				const AEVec2 transitionSpawn =
					ComputeTransitionSpawn(previousRoom, nextRoom, previousPos);

				blockedReturnDir = cameFrom;
				BuildCurrentRoom(cameFrom, &transitionSpawn);

				roomTransitionLocked = true;
				return;
			}
			else
			{
				//ClampPlayerInsideCurrentRoom();
				roomTransitionLocked = true;
				return;
			}
		}
	}
	camera.Update();

	AEVec2 p = player.GetPosition();
	enemyMgr.UpdateAll(p, player.GetIsFacingRight(), map);

	const AEVec2 pPos = player.GetPosition();
	const AEVec2 pSize = player.GetStats().playerSize;

	attackSystem.UpdateEnemyAttack(player, enemyMgr, activeBoss, map);

	trapMgr.Update(dt, player);

	testParticleSystem.SetSpawnRate(AEInputCheckCurr(AEVK_F) ? 2000.f : 0.f);
	if (AEInputCheckTriggered(AEVK_G))
		testParticleSystem.SpawnParticleBurst(300);
	testParticleSystem.Update();

	UI::GetDamageTextSpawner().Update();
	UI::Update();
	//std::cout << "MASTER VOL : " << AudioManager::GetMasterVolume()
	//		  << "BGM VOL : " << AudioManager::GetMusicVolume()
	//		  << "SFX VOL : " << AudioManager::GetSFXVolume() << '\n';

	//if (roomMgr.GetCurrentRoomID() == ROOM_1) { // To change to ROOM_10 after spawning is done
	//	AudioManager::PlayBossMusic(enemyBoss, roomMgr);
	//}
	AudioManager::Update();
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
	if (activeBoss)
		activeBoss->Render();
	//enemyBoss.Render();
	enemyMgr.RenderAll();
	attackSystem.Render();
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
			AudioManager::UnmuffleGameMusic();
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
	// ============================== Active Buffs (top-right) ==============================
	// Draw only existing buffs. 1 row max, 4 cards per row. No placeholders.
	const auto& buffs = BuffCardManager::GetCurrentBuffs();
	if (!buffs.empty())
	{
		const int cols = 3;

		// Bigger cards
		const float cardW = 180.0f;
		const float cardH = 255.0f;
		const float gapX = 20.0f;   // horizontal gap 
		const float gapY = 20.0f;   // vertical gap between rows 

		// Anchor: move this block to the right & top area (match your red mark)
		// (0,0) is top-left in pixel coordinates
		const float anchorX = w * 0.56f;   // increase => move right, decrease => move left
		const float anchorY = 110.0f;      // increase => move down, decrease => move up

		// Title position (aligned with cards)
		DrawTextPx(pauseFontLarge, "ACTIVE BUFFS:", anchorX, 80.0f, 0.75f, 1, 1, 1, 1);

		const int count = (int)buffs.size();
		const int drawCount = count;


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

				f32 red{}, green{}, blue{};
				switch (b.rarity) { // Match sprite hex colors
				case (RARITY_UNCOMMON):
					red = 0.015f;
					green = 1.0f;
					blue = 0.0f;
					break;
				case(RARITY_RARE):
					red = 0.0f;
					green = 0.384f;
					blue = 1.0f;
					break;
				case(RARITY_EPIC):
					red = 0.584f;
					green = 0.0f;
					blue = 1.0f;
					break;
				case(RARITY_LEGENDARY):
					red = 1.0f;
					green = 0.733f;
					blue = 0.0f;
					break;
				}

				// Title (m04)
				DrawTextPx(
					pauseFontSmall,
					b.cardName,
					120.0f, h - 125.0f, 1.0f,
					red, green, blue, 1
				);

				// Description/effect (Pixellari) - using cardEffect if available, otherwise fallback to cardDesc. 
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
	// ======================================================================================
}