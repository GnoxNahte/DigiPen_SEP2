/*!
@file   ItemDropManager.h
@author lim kang ping
@brief  This file declares the ItemDropManager class.
It manages item drops, their states, their movement,
and how they interact with the player and UI.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
#pragma once

#include <vector>

#include <AEVec2.h>
#include "../../Utils/Sprite.h"

class Player;

class ItemDropManager
{
public:
    enum class DropType
    {
        Heart
    };

    enum class State
    {
		LaunchFromEnemy,
        WorldIdle,
        CollectedToPlayer,
        FlyingToUI
    };

    struct Config
    {
        //const char* heartSpritePath = "Assets/Craftpix/heart.png";

        float renderScale = 1.5f;
        float bobAmplitude = 0.10f;
        float bobSpeed = 3.5f;

        float pickupRadius = 0.85f;
        float magnetRadius = 1.50f;
        float magnetSpeed = 15.0f;

        int healAmount = 2;
    };

    struct ItemDrop
    {
        DropType type = DropType::Heart;
        State state = State::WorldIdle;

        AEVec2 spawnPosition{ 0.f, 0.f };
        AEVec2 position{ 0.f, 0.f };
        AEVec2 velocity{ 0.f, 0.f };
        AEVec2 uiPosition{ 0.f, 0.f };
	

        float age = 0.f;
		float angle = 0.f;
        int healAmount = 0;
        bool active = true;
        float launchTime = 0.f;
        
    };

public:
    explicit ItemDropManager(const Config& cfg = Config{});
    ~ItemDropManager();

    void Init();
    void Exit();

    void Clear();

    void Update(Player& player);
    void Render();

    void SpawnHeart(const AEVec2& worldPos, int healAmount);

private:
    void UpdateDrop(ItemDrop& drop, Player& player, float dt);
    bool CheckPlayerPickup(const ItemDrop& drop, const Player& player) const;
    void RenderDrop(const ItemDrop& drop);

private:
    Config cfg;
    Sprite heartSprite;
    std::vector<ItemDrop> drops;
    int enemyKilledEventId = -1;
};