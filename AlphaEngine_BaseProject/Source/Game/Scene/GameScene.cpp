#include "GameScene.h"
#include "../../Utils/QuickGraphics.h"
#include "../../Utils/AEExtras.h"
#include "../Time.h"


//AABB collision helper
/*static bool AABB_Overlap(const AEVec2& aPos, const AEVec2& aSize,
	const AEVec2& bPos, const AEVec2& bSize)
{
	const float dx = std::fabs(aPos.x - bPos.x);
	const float dy = std::fabs(aPos.y - bPos.y);
	return dx <= (aSize.x + bSize.x) * 0.5f
		&& dy <= (aSize.y + bSize.y) * 0.5f;
}*/


namespace
{
	// squared-distance check (no sqrt)
	bool IsNear(const AEVec2& a, const AEVec2& b, float range)
	{
		float dx = b.x - a.x;
		float dy = b.y - a.y;
		return (dx * dx + dy * dy) <= (range * range);
	}
}


GameScene::GameScene() : 
	map(50, 50),
	player(&map, &enemyMgr),
	camera({ 1,1 }, { 49, 49 }, 64),
	testParticleSystem(
		20, 
		ParticleSystem::EmitterSettings{ 
			.angleRange{ PI / 3, PI / 4 },
			.speedRange{ 30.f, 50.f },
			.lifetimeRange{1.f, 2.f},
		} 
	),
	enemyBoss(35, 2.90f)
{
	camera.SetFollow(&player.GetPosition(), 0, 50, true);
	// =============================== Traps Setup (debug) ===========================
	//auto& plate = trapMgr.Spawn<PressurePlate>(
	//	Box{ {2.f, 4.f}, {4.f, 1.f} }   
	//);

	//auto& spikes = trapMgr.Spawn<SpikePlate>(
	//	Box{ {6.5f, 4.f}, {3.f, 1.f} }, 
	//	1.f, 1.f, 10, true
	//);
	// 
	//plate.AddLinkedTrap(&spikes);

	//trapMgr.Spawn<LavaPool>(
	//	Box{ {2.f, 3.0f}, {4.f, 0.8f} }, 
	//	2, 0.2f
	//);

	
}

GameScene::~GameScene()
{
}

void GameScene::Init()
{
	player.Reset(AEVec2{ 2, 2 });
	std::vector<EnemyManager::SpawnInfo> spawns;
	spawns.push_back({ Enemy::Preset::Druid, {30.f, 3.f} });
	spawns.push_back({ Enemy::Preset::Skeleton, {34.f, 3.f} });
	enemyMgr.SetBoss(&enemyBoss);
	enemyMgr.SetSpawns(spawns);
	enemyMgr.SpawnAll();

}

void GameScene::Update()
{
	player.Update();
	camera.Update();

	enemyMgr.UpdateAll(player.GetPosition());

	AEVec2 p = player.GetPosition();
	enemyBoss.Update(p, player.IsFacingRight());

	const AEVec2 pPos = player.GetPosition();
	const AEVec2 pSize = player.GetStats().playerSize;

	attackSystem.ApplyEnemyAttacksToPlayer(player, enemyMgr, &enemyBoss);

	// --- DELETE THIS LATER, PREVIOUS ENEMY ATTACK PLAYER!!!!! -------
	//const AEVec2 pPos = player.GetPosition();
	//const AEVec2 pSize = player.GetStats().playerSize;
	/*
	// Boss normal melee attack (ATTACK state via EnemyAttack)
	if (enemyBoss.PollAttackHit())
	{
		// EnemyAttack already checked absDx <= hitRange at hit moment (because Boss uses absDx in attack.Update).
		// Add a Y tolerance so it doesn't hit through platforms.
		const float dy = std::fabs(pPos.y - enemyBoss.position.y);
		const float yTol = (pSize.y * 0.5f) + 0.6f; // tune 0.3~1.0 depending on your level scale

		if (dy <= yTol)
		{
			player.TakeDamage(enemyBoss.attackDamage, enemyBoss.position); // choose your boss damage
			std::cout << "[Boss] HIT player (melee)\n";
		}
	}
	
	// Boss special spell damage
	const int spellHits = enemyBoss.ConsumeSpecialHits(player.GetPosition(),
	player.GetStats().playerSize);
	if (spellHits > 0)
	{
		const int spellDmg = 1;                 // tune
		player.TakeDamage(spellHits * spellDmg, enemyBoss.position);
		std::cout << "[Boss] spell hit x" << spellHits << "\n";
	}

	enemyMgr.ForEachEnemy([&](Enemy& e)
		{
			// do player/enemy AABB checks here
			// if hit: e.ApplyDamage(1);

			if (!e.PollAttackHit()) return;

			const AEVec2 ePos = e.GetPosition();

			const float dx = std::fabs(pPos.x - ePos.x);
			const float dy = std::fabs(pPos.y - ePos.y);

			// mid/close range on X, plus a small Y tolerance so it doesn't hit through floors
			if (dx <= e.GetAttackHitRange() && dy <= (pSize.y * 0.5f + 0.6f))
			{
				player.TakeDamage(e.GetAttackDamage(), e.GetPosition());
				std::cout << "Enemy HIT player!\n";
			}
		});
	*/

	float dt = static_cast<float>(Time::GetInstance().GetScaledDeltaTime());
	trapMgr.Update(dt, player);

	//std::cout << std::fixed << std::setprecision(2) << AEFrameRateControllerGetFrameRate() << std::endl;

	// === Particle system test ===
	testParticleSystem.SetSpawnRate(AEInputCheckCurr(AEVK_F) ? 2000.f : 0.f);
	//testParticleSystem.SetSpawnRate(AEInputCheckCurr(AEVK_F) ? 5000000.f : 0.f);
	if (AEInputCheckTriggered(AEVK_G))
		testParticleSystem.SpawnParticleBurst( 300);
	testParticleSystem.Update();
}

void GameScene::Render()
{
	map.Render();
	//trapMgr.Render();   // for debug
	testParticleSystem.Render();
	player.Render();
	enemyBoss.Render();
	enemyMgr.RenderAll();


	// === Code below is for DEBUG ONLY ===

	// === Debug Info ===
	//Box playerBox{ player.GetPosition(), player.GetSize() };

	//Box plateBox{ {2.f, 4.f}, {4.f, 1.f} };
	//Box spikeBox{ {6.5f, 4.f}, {3.f, 1.f} };
	//Box lavaBox{ {2.f, 3.0f}, {4.f, 0.8f} };


	//bool onPlate = IntersectsBox(playerBox, plateBox);
	//bool onSpike = IntersectsBox(playerBox, spikeBox);
	//bool onLava = IntersectsBox(playerBox, lavaBox);

	//std::string ov = "Overlap: Plate=" + std::to_string(onPlate)
	//	+ " Spike=" + std::to_string(onSpike)
	//	+ " Lava=" + std::to_string(onLava);
	//QuickGraphics::PrintText(ov.c_str(), -1, 0.60f, 0.3f, 0.5f, 0.5f, 0.5f, 1);

	


	AEVec2 worldMousePos;
	AEExtras::GetCursorWorldPosition(worldMousePos);
	std::string str = "World Mouse Pos:" + std::to_string(worldMousePos.x) + ", " + std::to_string(worldMousePos.y);
	QuickGraphics::PrintText(str.c_str(), -1, 0.95f, 0.3f, 0.5f, 0.5f, 0.5f, 1);
	str = "FPS:" + std::to_string(AEFrameRateControllerGetFrameRate());
	QuickGraphics::PrintText(str.c_str(), -1, 0.90f, 0.3f, 0.5f, 0.5f, 0.5f, 1);


	str = "Time:" + std::to_string(Time::GetInstance().GetScaledElapsedTime());
	QuickGraphics::PrintText(str.c_str(), -1, 0.85f, 0.3f, 0.5f, 0.5f, 0.5f, 1);

	std::string ppos = "Player Pos: " + std::to_string(player.GetPosition().x) + ", " + std::to_string(player.GetPosition().y);
	QuickGraphics::PrintText(ppos.c_str(), -1, 0.80f, 0.3f, 0.5f, 0.5f, 0.5f, 1);

	if (AEInputCheckTriggered(AEVK_R))
		GSM::ChangeScene(SceneState::GS_GAME);
	else if (AEInputCheckTriggered(AEVK_T))
		GSM::ChangeScene(SceneState::GS_MAIN_MENU);
	else if (AEInputCheckTriggered(AEVK_Y))
		GSM::ChangeScene(SceneState::GS_LEVEL_EDITOR);
}

void GameScene::Exit()
{
}
