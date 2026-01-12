#pragma once
#include "Player/Player.h"
#include "enemy/EnemyA.h"
#include "Camera.h"

// Temp - stores in game data
// Change to a proper game state manager
class GameScene
{
public:
	GameScene();
	~GameScene();
	void Update();
	void Render();
private:
	Player player;
	EnemyA enemy;
	Camera camera;
};

