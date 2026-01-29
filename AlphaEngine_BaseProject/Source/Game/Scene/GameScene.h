#pragma once
#include "../Player/Player.h"
#include "../Camera.h"
#include "../Environment/MapGrid.h"
#include "../../Utils/ParticleSystem.h"
#include "../Environment/traps.h"
#include "GSM.h"
#include "../enemy/EnemyA.h"
#include "../enemy/EnemyBoss.h"


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
	EnemyA enemyA;
	EnemyBoss enemyBoss;
	MapGrid map;
	ParticleSystem testParticleSystem;
	TrapManager trapMgr;
};

