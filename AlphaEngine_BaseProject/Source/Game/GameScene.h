#pragma once
#include "Player/Player.h"
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
	Camera camera;
};

