/*!
@file   Enemy.h
@author lim kang ping
@brief  This file declares the Enemy class.
It defines regular enemy data and functions for movement,
attacks, damage, animation, and rendering.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/

#pragma once

#include "../../Utils/Sprite.h"
#include "EnemyAttack.h"
#include <AEVec2.h>
#include "IDamageable.h"
#include "../../Editor/EditorUtils.h"
#include "../../CommonTypes.h"
#include "../../Utils/ParticleSystem.h"
#include "EnemyStats.h"

class MapGrid; 

class Enemy : public IDamageable, Inspectable
{
public:
    enum class Preset
    {
        Druid,
        Skeleton
    };

public:
    Enemy(Preset preset = Preset::Druid, float initialPosX = 0.f, float initialPosY = 0.f);
    ~Enemy() = default;

    int GetMaxHp() const { return stats.maxHp; }
    int GetCurrentHp() const { return hp; }

    void SetMaxHp(int value) { stats.maxHp = value; }
    void SetCurrentHp(int value) { hp = value; }
    void SetAttackDamage(int value) { stats.attackDamage = value; }

    void ApplyRoomScaling(int extraHp, int extraDamage);

    virtual const AEVec2& GetHurtboxPos() const override
    {
        return GetPosition();
    }

    virtual const AEVec2& GetHurtboxSize() const override
    {
        return GetSize();
    }

    void DrawInspector() override;
    bool CheckIfClicked(const AEVec2& mousePos) override;

    void Update(const AEVec2& playerPos, MapGrid& map);
    void Render();

    bool PollAttackHit() { return !dead && attack.PollHit(); }

    Preset GetPreset() const { return presetType; }
    bool IsDruid() const { return presetType == Preset::Druid; }

    bool IsDead() const { return dead; }
    int GetHP() const { return hp; }

    bool TryTakeDamage(int dmg, const AEVec2& hitOrigin, DAMAGE_TYPE type = DAMAGE_TYPE_NORMAL) override;

    const AEVec2& GetPosition() const { return position; }
    const AEVec2& GetSize() const { return size; }
    bool IsChasing() const { return chasing; }
    bool IsReturningHome() const { return returningHome; }

    float GetAttackHitRange() const { return attack.hitRange; }
    int GetAttackDamage() const { return stats.attackDamage; }

    void SetDebugDraw(bool on) { debugDraw = on; }
    AEVec2 hurtboxOffset{ 0.f, 0.f };

    ParticleSystem particleSystem{ 30, {} };
    ParticleSystem castParticleSystem{ 40, {} };

    bool wasDruidCasting = false;
    float druidCastFxTimer = 0.f;

    bool HasLockedDruidSpellTarget() const { return druidSpellTargetLocked; }
    const AEVec2& GetLockedDruidSpellTarget() const { return druidSpellTargetPos; }
    void ClearLockedDruidSpellTarget() { druidSpellTargetLocked = false; }

private:
    void UpdateAnimation();
    static float GetAnimDurationSec(const Sprite& sprite, int stateIndex);
    static const char* GetStatsFilePath(Preset preset);

    Preset presetType = Preset::Skeleton;

    EnemyStats stats;
    Sprite sprite;
    EnemyAttack attack;

    AEVec2 position{ 0.f, 0.f };
    AEVec2 homePos{ 0.f, 0.f };
    AEVec2 velocity{ 0.f, 0.f };
    AEVec2 size{ 0.8f, 0.8f };
    AEVec2 facingDirection{ 1.f, 0.f };

    bool chasing{ false };
    bool returningHome{ false };
    bool hadAggro = false;
    bool debugDraw{ false };

    float idleWalkLeft = 0.f;
    float idlePauseLeft = 0.f;
    float idleDirX = 1.f;
    float idleSpeedMul = 0.35f;

    int hp{ 1 };
    bool dead{ false };

    float hurtTimeLeft{ 1.0f };
    float deathTimeLeft{ 0.5f };
    bool hidden = false;
    int lastHitAttackId{ -1 };

    bool HasGroundAhead(MapGrid& map, float dirX) const;
    bool HasWallAhead(MapGrid& map, float dirX) const;
    bool HasLineOfSightToTarget(MapGrid& map, const AEVec2& targetPos) const;

    bool targetLocked = false;
    float losGraceTimer = 0.f;

    AEVec2 druidSpellTargetPos{ 0.f, 0.f };
    bool druidSpellTargetLocked = false;
};