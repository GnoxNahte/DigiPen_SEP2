#pragma once
#include "../Player/Player.h"
#include "../Camera.h"
#include "../Environment/MapGrid.h"
#include "../../Utils/ParticleSystem.h"
#include "GSM.h"

class GameScene : public BaseScene
{
public:
	GameScene();
	~GameScene();
	void Init() override;
	void Update() override;
	void Render() override;
	void Exit() override;
private:
	Player player;
	Camera camera;
	MapGrid map;
	ParticleSystem testParticleSystem;
};

