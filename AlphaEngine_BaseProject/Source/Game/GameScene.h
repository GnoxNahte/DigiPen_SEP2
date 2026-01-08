#pragma once
#include "Player/Player.h"
#include "Camera.h"

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

