#include "GameScene.h"

GameScene::GameScene() : camera(0, 0, 2)
{
	camera.SetFollow(&player.position, 0, 50);
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
	player.Render();

}
