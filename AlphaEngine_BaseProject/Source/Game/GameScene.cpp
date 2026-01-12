#include "GameScene.h"
#include "../Utils/QuickGraphics.h"

GameScene::GameScene() : camera(0, 0, 64), player(0, 1)
{
	camera.SetFollow(&player.position, 0, 50, true);

	enemies[0] = Enemy1(-3.0f, 1.0f);
	enemies[1] = Enemy1(-1.5f, 1.0f);
	enemies[2] = Enemy1(1.5f, 1.0f);
	enemies[3] = Enemy1(3.0f, 1.0f);
	enemies[4] = Enemy1(0.0f, 2.5f);
}

GameScene::~GameScene()
{
}

void GameScene::Update()
{
	camera.Update();
	player.Update();

	for (auto& e : enemies)
		e.Update(player.position); // enemy chases player
}

void GameScene::Render()
{
	//QuickGraphics::DrawRect(0.f, -1.f, 10.f, 1.f, 0x11FFFFFF);
	QuickGraphics::DrawRect(0.f, -1.5f, 10.f, 0.5f, 0x11FFFFFF);
	//QuickGraphics::DrawRect(0.f, 0.f, 1.f, 1.f, 0xFFFF0000);
	player.Render();

	for (const auto& e : enemies)
		e.Render();
}
