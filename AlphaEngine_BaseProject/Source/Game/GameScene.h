#pragma once
#include "Player/Player.h"

class GameScene
{
public:
	GameScene();
	~GameScene();
	void Update();
	void Render();
private:
	Player player;
};

