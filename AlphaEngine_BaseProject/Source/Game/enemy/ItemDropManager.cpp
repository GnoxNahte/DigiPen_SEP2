/*!
@file   ItemDropManager.cpp
@author lim kang ping
@brief  This file implements the ItemDropManager class.
It handles spawning, updating, collecting, and clearing item drops,
such as hearts dropped by enemies.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/

#include "ItemDropManager.h"
#include "../Player/Player.h"
#include "../../Utils/Event/EventSystem.h"
#include "../../Utils/AEExtras.h"
#include "../Camera.h"
#include "../Time.h"
#include "../../Utils/Sprite.h" 
#include "../UI.h"
#include "../AudioManager.h"

namespace
{
    float LengthSquared(const AEVec2& v)
    {
        return v.x * v.x + v.y * v.y;
    }

    AEVec2 NormalizedOrZero(const AEVec2& v)
    {
        const float lenSq = LengthSquared(v);
        if (lenSq <= 0.000001f)
            return { 0.f, 0.f };

        const float invLen = 1.0f / sqrtf(lenSq);
        return { v.x * invLen, v.y * invLen };
    }
}

ItemDropManager::ItemDropManager(const Config& cfgIn)
    : cfg(cfgIn)
    , heartSprite("Assets/Craftpix/hearts.png")
{
}

ItemDropManager::~ItemDropManager()
{
    Exit();
}

void ItemDropManager::Init()
{
    if (enemyKilledEventId >= 0)
        return;

    enemyKilledEventId =
        EventSystem::Subscribe<IDamageable::EnemyKilledEvent>(
            [this](const IDamageable::EnemyKilledEvent& ev)
            {
                for (int i = 0; i < ev.count; ++i)
                {
                    AEVec2 offset = AEVec2{
                        AEExtras::RandomRange({ -0.3f, 0.3f }),
                        AEExtras::RandomRange({ -0.1f, 0.2f })
                    };
                    SpawnHeart(ev.position + offset, cfg.healAmount);
                }
            });
}

void ItemDropManager::Exit()
{
    if (enemyKilledEventId >= 0)
    {
        EventSystem::Unsubscribe<IDamageable::EnemyKilledEvent>(enemyKilledEventId);
        enemyKilledEventId = -1;
    }

    Clear();
}

void ItemDropManager::Clear()
{
    drops.clear();
}

void ItemDropManager::SpawnHeart(const AEVec2& worldPos, int healAmount)
{
    ItemDrop drop;
    drop.type = DropType::Heart;
    drop.state = State::LaunchFromEnemy;
	drop.launchTime = 0.f;
    drop.spawnPosition = worldPos;
    drop.position = worldPos;

    drop.velocity = AEVec2{
        AEExtras::RandomRange({ -1.f, 1.f }),
        AEExtras::RandomRange({ 3.f, 4.0f })
    };

    drop.age = 0.f;
    drop.healAmount = healAmount;
    drop.active = true;

    drops.push_back(drop);
}

void ItemDropManager::Update(Player& player)
{
    const float dt = static_cast<float>(Time::GetInstance().GetScaledDeltaTime());
    heartSprite.Update();

    for (ItemDrop& drop : drops)
    {
        if (!drop.active)
            continue;

        UpdateDrop(drop, player, dt);
    }

    drops.erase(
        std::remove_if(drops.begin(), drops.end(),
            [](const ItemDrop& d) { return !d.active; }),
        drops.end());
}

void ItemDropManager::UpdateDrop(ItemDrop& drop, Player& player, float dt)
{
    drop.age += dt;

    switch (drop.state)
    {
    case State::LaunchFromEnemy:
    {
        drop.launchTime += dt;

        const float duration = 0.35f;
        float t = drop.launchTime / duration;
        if (t > 1.f) t = 1.f;

        // small horizontal push
        float offsetX = drop.velocity.x * t;

        // arc: 0 -> peak -> 0
        float arcY = 1.2f * 4.f * t * (1.f - t);

        drop.position.x = drop.spawnPosition.x + offsetX;
        drop.position.y = drop.spawnPosition.y + arcY;

        if (t >= 1.f)
        {
            drop.velocity = { 0.f, 0.f };
            drop.state = State::WorldIdle;
        }
        break;
    }
    case State::WorldIdle:
    {
        if (CheckPlayerPickup(drop, player))
        {
            drop.state = State::CollectedToPlayer;
        }
        break;
    }

    case State::CollectedToPlayer:
    {
        AEVec2 toPlayer = player.GetPosition() - drop.position;
        AEVec2 dir = NormalizedOrZero(toPlayer);

        drop.velocity = dir * cfg.magnetSpeed;
        drop.position += drop.velocity * dt;

        const AEVec2 remain = player.GetPosition() - drop.position;
        if (LengthSquared(remain) <= 0.10f * 0.10f)
        {
            drop.uiPosition = AEVec2{
            drop.position.x * Camera::scale + Camera::position.x,
            drop.position.y * Camera::scale + Camera::position.y
            };

            drop.state = State::FlyingToUI;

        }
		AudioManager::PlaySFX(*AudioManager::healthPickup, AudioManager::GetSFXVolume());
        break;
    }

    case State::FlyingToUI:
    {
        AEVec2 target = UI::GetHealthBarHeartTargetPx();
        AEVec2 toTarget = target - drop.uiPosition;
        AEVec2 dir = NormalizedOrZero(toTarget);

        const float uiFlySpeed = 900.f; // tune
        drop.uiPosition += dir * uiFlySpeed * dt;

        if (LengthSquared(target - drop.uiPosition) <= 12.f * 12.f)
        {
            player.Heal(drop.healAmount);
            drop.active = false;
        }
        break;
    }


    }
}

bool ItemDropManager::CheckPlayerPickup(const ItemDrop& drop, const Player& player) const
{
    AEVec2 delta = player.GetPosition() - drop.position;
    const float r = cfg.pickupRadius;
    return LengthSquared(delta) <= r * r;
}

void ItemDropManager::Render()
{
    for (const ItemDrop& drop : drops)
    {
        if (!drop.active)
            continue;

        RenderDrop(drop);
    }
}

void ItemDropManager::RenderDrop(const ItemDrop& drop)
{
    if (drop.state == State::FlyingToUI)
    {
        AEMtx33 transform;
        AEMtx33Identity(&transform);

        const float pxSize = 150.f; // tune this
        AEMtx33Scale(&transform, pxSize, pxSize);

        // uiPosition is already in pixel space
        AEMtx33TransApply(
            &transform, &transform,
            drop.uiPosition.x,
            drop.uiPosition.y
        );

        AEGfxSetTransform(transform.m);
        heartSprite.Render();
        return;
    }

    AEVec2 renderPos = drop.position;

    if (drop.state == State::WorldIdle)
    {
        renderPos.y += sinf(drop.age * cfg.bobSpeed) * cfg.bobAmplitude;
    }

    AEMtx33 transform;
    AEMtx33Identity(&transform);

    AEMtx33Scale(&transform, cfg.renderScale + 1.f, cfg.renderScale);
    AEMtx33TransApply(&transform, &transform,
        renderPos.x - (0.5f - heartSprite.metadata.pivot.x),
        renderPos.y + (0.5f - heartSprite.metadata.pivot.y));

    AEMtx33ScaleApply(&transform, &transform, Camera::scale, Camera::scale);
    AEGfxSetTransform(transform.m);

    heartSprite.Render();
}