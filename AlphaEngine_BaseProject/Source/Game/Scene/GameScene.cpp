#include "GameScene.h"
#include "../../Utils/QuickGraphics.h"
#include "../../Utils/AEExtras.h"

GameScene::GameScene() : 
	player(&map), 
	map(50, 50),
	camera({ 1,1 }, { 49, 49 }, 64),
	testParticleSystem(20)
{
	camera.SetFollow(&player.position, 0, 50, true);
	testParticleSystem.Init();
	testParticleSystem.SetSpawnRate(0.f);
}

GameScene::~GameScene()
{
}

void GameScene::Init()
{
	player.Reset(AEVec2{ 2, 4 });
}

void GameScene::Update()
{
	camera.Update();
	player.Update();
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
	player.Render();

	// === Code below is for DEBUG ONLY ===
	testParticleSystem.Render();

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
