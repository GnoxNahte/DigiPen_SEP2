#include "GameScene.h"
#include "../Utils/QuickGraphics.h"
#include "../Utils/AEExtras.h"

GameScene::GameScene() : 
	player(&map, 2, 4), 
	map(50, 50),
	camera({ 1,1 }, { 49, 49 }, 64)
{
	camera.SetFollow(&player.position, 0, 50, true);
}

GameScene::~GameScene()
{
}

void GameScene::Update()
{
	camera.Update();
	player.Update();
	//std::cout << std::fixed << std::setprecision(2) << AEFrameRateControllerGetFrameRate() << std::endl;
}

void GameScene::Render()
{
	
	//QuickGraphics::DrawRect(0.f, -1.f, 10.f, 1.f, 0x11FFFFFF);
	//QuickGraphics::DrawRect(0.f, -1.5f, 10.f, 0.5f, 0x11FFFFFF);
	//QuickGraphics::DrawRect(0.f, 0.f, 1.f, 1.f, 0xFFFF0000);

	map.Render(camera);
	player.Render();

	AEVec2 worldMousePos;
	AEExtras::GetCursorWorldPosition(worldMousePos, camera.position);
	std::string str = "World Mouse Pos:" + std::to_string(worldMousePos.x) + ", " + std::to_string(worldMousePos.y);
	QuickGraphics::PrintText(str.c_str(), -1, 0.95f, 0.3f, 1, 1, 1, 1);
	str = "FPS:" + std::to_string(AEFrameRateControllerGetFrameRate());
	QuickGraphics::PrintText(str.c_str(), -1, 0.90f, 0.3f, 1, 1, 1, 1);
}
