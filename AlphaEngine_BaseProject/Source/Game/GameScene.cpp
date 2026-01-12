#include "GameScene.h"
#include "../Utils/QuickGraphics.h"
#include "../Utils/AEExtras.h"

GameScene::GameScene() : camera(0, 0, 64), player(&map, -1, 2), map(10, 10)
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
}

void GameScene::Render()
{
	
	//QuickGraphics::DrawRect(0.f, -1.f, 10.f, 1.f, 0x11FFFFFF);
	//QuickGraphics::DrawRect(0.f, -1.5f, 10.f, 0.5f, 0x11FFFFFF);
	//QuickGraphics::DrawRect(0.f, 0.f, 1.f, 1.f, 0xFFFF0000);


	map.Render();
	player.Render();

	AEVec2 worldMousePos;
	AEExtras::GetCursorWorldPosition(worldMousePos, camera.position);
	std::string str = "World Mouse Pos:" + std::to_string(worldMousePos.x) + ", " + std::to_string(worldMousePos.y);
	QuickGraphics::PrintText(str.c_str(), -1, 0.95f, 0.3f, 1, 1, 1, 1);
}
