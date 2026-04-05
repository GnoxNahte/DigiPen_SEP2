/*!
@file   IDamageable.h
@author lim kang ping
@brief  This file declares the IDamageable interface.
It is used for objects/enemies that can take damage, provide hurtbox data,
and report whether they are dead.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
#pragma once
#include <AEVec2.h>
#include "../../CommonTypes.h"

class IDamageable
{
public:
    virtual ~IDamageable() = default;

    virtual const AEVec2& GetHurtboxPos()  const = 0;
    virtual const AEVec2& GetHurtboxSize() const = 0;

    virtual bool IsDead() const = 0;

    // Return true if damage was applied (not invuln, not already hit, etc.)
    virtual bool   TryTakeDamage(int dmg, const AEVec2& hitOrigin, DAMAGE_TYPE type = DAMAGE_TYPE_NORMAL) = 0;

    struct EnemyKilledEvent
    {
        AEVec2 position; // where the enemy died , maybe for item drop later?
        int count = 1;
    };
};