#include "GameScene.h"
#include "../Utils/QuickGraphics.h"
#include "../Utils/AEExtras.h"


GameScene::GameScene() : 
	player(&map, 2, 4), 
	map(50, 50),
	camera({ 1,1 }, { 49, 49 }, 64),
	testParticleSystem(20)
{
	camera.SetFollow(&player.position, 0, 50, true);
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

void GameScene::Update()
{
	camera.Update();
	player.Update();
	float dt = (float)AEFrameRateControllerGetFrameTime();
	trapMgr.Update(dt, player);

	//std::cout << std::fixed << std::setprecision(2) << AEFrameRateControllerGetFrameRate() << std::endl;

	// === Particle system test ===
	testParticleSystem.SetSpawnRate(AEInputCheckCurr(AEVK_F) ? 10000.f : 0.f);
	if (AEInputCheckTriggered(AEVK_G))
		testParticleSystem.SpawnParticleBurst({ 2,2 }, 300);
	testParticleSystem.Update();
}

void GameScene::Render()
{
	
	//QuickGraphics::DrawRect(0.f, -1.f, 10.f, 1.f, 0x11FFFFFF);
	//QuickGraphics::DrawRect(0.f, -1.5f, 10.f, 0.5f, 0x11FFFFFF);
	//QuickGraphics::DrawRect(0.f, 0.f, 1.f, 1.f, 0xFFFF0000);

	map.Render(camera);
	//trapMgr.Render();   // for debug
	player.Render();
	testParticleSystem.Render();

	// === Debug Info ===
	std::string hp = "HP: " + std::to_string(player.GetHP());
	QuickGraphics::PrintText(hp.c_str(), -1, 0.85f, 0.3f, 0.5f, 0.5f, 0.5f, 1);

	std::string ppos = "Player Pos: " + std::to_string(player.position.x) + ", " + std::to_string(player.position.y);
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
}
