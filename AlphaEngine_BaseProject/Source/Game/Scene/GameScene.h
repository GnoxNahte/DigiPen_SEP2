#pragma once
#include "../Player/Player.h"
#include "../Camera.h"
#include <cmath>
#include "../Environment/MapGrid.h"
#include "../../Utils/ParticleSystem.h"
#include "../Environment/traps.h"
#include "GSM.h"
#include "../enemy/EnemyA.h"
#include "../enemy/EnemyB.h"
#include "../enemy/EnemyBoss.h"
#include "../enemy/Enemy.h"


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
	MapGrid map;
	Player player;
	Camera camera;
	//EnemyA enemyA;
	//EnemyB enemyB;
	EnemyBoss enemyBoss;

	ParticleSystem testParticleSystem;
	TrapManager trapMgr;

	Enemy enemyA;
	Enemy enemyB;
};

