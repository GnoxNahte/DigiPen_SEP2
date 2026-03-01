#pragma once
#include <AEVec2.h>

class IDamageable
{
public:
    virtual ~IDamageable() = default;

    virtual const AEVec2& GetHurtboxPos()  const = 0;
    virtual const AEVec2& GetHurtboxSize() const = 0;

    virtual bool IsDead() const = 0;

    // Return true if damage was applied (not invuln, not already hit, etc.)
    virtual bool   TryTakeDamage(int dmg, const AEVec2& hitOrigin) = 0;
};