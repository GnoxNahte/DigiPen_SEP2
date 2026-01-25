#include "GameScene.h"
#include "../Utils/QuickGraphics.h"
#include "../Utils/AEExtras.h"
#include "../Game/enemy/EnemyA.h"
#include "../Game/enemy/EnemyBoss.h"
#include "Environment/MapGrid.h"
#include <iostream>


GameScene::GameScene()
    : map(50, 50)
    , player(&map, 2, 4)
    , enemy(30,3)
    , camera({ 1, 1 }, { 49, 49 }, 64)
	, enemyBoss(35, 3)
{
    camera.SetFollow(&player.position, 0, 50, true);
}

GameScene::~GameScene()
{
}

void GameScene::Update()
{
    camera.Update();
    player.Update();
    enemyBoss.Update(player.position);
    enemy.Update(player.position);
    if (enemy.PollAttackHit())
    {
        // later: apply player damage
        // for now: print / debug
        std::cout << "Enemy HIT!\n";
    }

    #include "../Game/enemy/EnemyA.h"
}

void GameScene::Render()
{
    map.Render(camera);
    player.Render();
    enemy.Render();
    enemyBoss.Render();
    

    AEVec2 worldMousePos;
    AEExtras::GetCursorWorldPosition(worldMousePos, camera.position);

    std::string str = "World Mouse Pos:" + std::to_string(worldMousePos.x) + ", " + std::to_string(worldMousePos.y);
    QuickGraphics::PrintText(str.c_str(), -1, 0.95f, 0.3f, 1, 1, 1, 1);

    str = "FPS:" + std::to_string(AEFrameRateControllerGetFrameRate());
    QuickGraphics::PrintText(str.c_str(), -1, 0.90f, 0.3f, 1, 1, 1, 1);
}
