#include "GameScene.h"
#include "../Utils/QuickGraphics.h"

GameScene::GameScene() : camera(0, 0, 64), player(0, 1)
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
	QuickGraphics::DrawRect(0.f, -1.5f, 10.f, 0.5f, 0x11FFFFFF);
	//QuickGraphics::DrawRect(0.f, 0.f, 1.f, 1.f, 0xFFFF0000);
	player.Render();
}
