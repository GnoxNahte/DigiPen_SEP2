#include "ItemDropManager.h"

#include "../Player/Player.h"
#include "../../Utils/Event/EventSystem.h"
#include "../../Utils/AEExtras.h"
#include "../Camera.h"
#include "../Time.h"
#include "../../Utils/Sprite.h" 
#include "../UI.h"

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
    enemyKilledEventId =
        EventSystem::Subscribe<IDamageable::EnemyKilledEvent>(
            [this](const IDamageable::EnemyKilledEvent& ev)
            {
                SpawnHeart(ev.position, cfg.healAmount);
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
    drop.state = State::WorldIdle;
    drop.spawnPosition = worldPos;
    drop.position = worldPos;
    drop.velocity = { 0.f, 0.f };
    drop.age = 0.f;
    drop.healAmount = healAmount;
    drop.active = true;

    drops.push_back(drop);
}

void ItemDropManager::Update(const Player& player)
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

void ItemDropManager::UpdateDrop(ItemDrop& drop, const Player& player, float dt)
{
    drop.age += dt;

    switch (drop.state)
    {
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
            // final heal trigger here if want to heal player from here...?
            // player.Heal(drop.healAmount, player.GetPosition());
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