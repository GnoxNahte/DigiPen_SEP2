#include "GameScene.h"
#include "../Utils/QuickGraphics.h"
#include "../Game/enemy/EnemyA.h"

GameScene::GameScene() : camera(0, 0, 64), player(0, 1), enemy(-3.f, 0.f)
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

	enemy.Update(player.position);
}

void GameScene::Render()
{
	//QuickGraphics::DrawRect(0.f, -1.f, 10.f, 1.f, 0x11FFFFFF);
	QuickGraphics::DrawRect(0.f, -1.5f, 10.f, 0.5f, 0x11FFFFFF);
	//QuickGraphics::DrawRect(0.f, 0.f, 1.f, 1.f, 0xFFFF0000);
	player.Render();
	enemy.Render();
}
