#include "GameScene.h"
#include "../../Utils/QuickGraphics.h"
#include "../../Utils/AEExtras.h"




GameScene::GameScene() : 
	map(50, 50),
	player(&map), 
	enemyA(30, 3),
	camera({ 1,1 }, { 49, 49 }, 64),
	testParticleSystem(20, {}),
	enemyBoss(35, 3)
{
	camera.SetFollow(&player.GetPosition(), 0, 50, true);
	// =============================== Traps Setup (debug) ===========================
	//auto& plate = trapMgr.Spawn<PressurePlate>(
	//	Box{ {2.f, 4.f}, {4.f, 1.f} }   
	//);

	//auto& spikes = trapMgr.Spawn<SpikePlate>(
	//	Box{ {6.5f, 4.f}, {3.f, 1.f} }, 
	//	1.f, 1.f, 10, true
	//);

	//plate.AddLinkedTrap(&spikes);

	//trapMgr.Spawn<LavaPool>(
	//	Box{ {2.f, 3.0f}, {4.f, 0.8f} }, 
	//	2, 0.2f
	//);


}

GameScene::~GameScene()
{
}

void GameScene::Init()
{
	player.Reset(AEVec2{ 2, 2 });
}

void GameScene::Update()
{
	camera.Update();
	player.Update();


	AEVec2 p = player.GetPosition();
	enemyA.Update(p);
	enemyBoss.Update(p);

	if (enemyA.PollAttackHit())
	{
		// later: apply player damage
		// for now: print / debug
		std::cout << "Enemy HIT!\n";
	}
	float dt = (float)AEFrameRateControllerGetFrameTime();
	trapMgr.Update(dt, player);

	//std::cout << std::fixed << std::setprecision(2) << AEFrameRateControllerGetFrameRate() << std::endl;

	// === Particle system test ===
	testParticleSystem.SetSpawnRate(AEInputCheckCurr(AEVK_F) ? 50000.f : 0.f);
	if (AEInputCheckTriggered(AEVK_G))
		testParticleSystem.SpawnParticleBurst({ 2,2 }, 300);
	testParticleSystem.Update();
}

void GameScene::Render()
{
	map.Render(camera);
	//trapMgr.Render();   // for debug
	player.Render();
	enemyA.Render();
	enemyBoss.Render();

	// === Code below is for DEBUG ONLY ===
	testParticleSystem.Render();

	// === Debug Info ===
	std::string hp = "HP: " + std::to_string(player.GetHealth());
	QuickGraphics::PrintText(hp.c_str(), -1, 0.85f, 0.3f, 0.5f, 0.5f, 0.5f, 1);

	std::string ppos = "Player Pos: " + std::to_string(player.GetPosition().x) + ", " + std::to_string(player.GetPosition().y);
	QuickGraphics::PrintText(ppos.c_str(), -1, 0.80f, 0.3f, 0.5f, 0.5f, 0.5f, 1);

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

	


	AEVec2 worldMousePos;
	AEExtras::GetCursorWorldPosition(worldMousePos, camera.position);
	std::string str = "World Mouse Pos:" + std::to_string(worldMousePos.x) + ", " + std::to_string(worldMousePos.y);
	QuickGraphics::PrintText(str.c_str(), -1, 0.95f, 0.3f, 0.5f, 0.5f, 0.5f, 1);
	str = "FPS:" + std::to_string(AEFrameRateControllerGetFrameRate());
	QuickGraphics::PrintText(str.c_str(), -1, 0.90f, 0.3f, 0.5f, 0.5f, 0.5f, 1);

	if (AEInputCheckTriggered(AEVK_R))
		GSM::ChangeScene(SceneState::GS_GAME);
	else if (AEInputCheckTriggered(AEVK_T))
		GSM::ChangeScene(SceneState::GS_MAIN_MENU);
}

void GameScene::Exit()
{
}
