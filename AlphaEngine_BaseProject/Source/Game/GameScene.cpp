#include "GameScene.h"

GameScene::GameScene()
{
}

GameScene::~GameScene()
{
}

void GameScene::Update()
{
	player.Update();
}

void GameScene::Render()
{
	player.Render();
}
