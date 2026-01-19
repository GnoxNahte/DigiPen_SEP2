#pragma once
#include <algorithm> // std::max

class EnemyAttack
{
public:
    // ---- Tuning ----
    float startRange = 2.0f;  // start attack if within this distance
    float hitRange = 1.0f;  // hit only if still within range at hit moment
    float cooldown = 0.8f;  // seconds between attack starts
    float hitTimeNormalized = 0.5f;  // 0..1 (hit time within animation)

    // Call every frame:
    //  - dt: delta time seconds
    //  - distanceToTarget: abs(x distance) or full distance, your choice (be consistent)
    //  - attackAnimDurationSec: duration of ATTACK animation in seconds (frameCount * timePerFrame)

    //if target goes beyond this while attacking, cancel attack
     float breakRange = 1.0f;   // set > startRange (hysteresis


    void Update(float dt, float distanceToTarget, float attackAnimDurationSec)
    {
        justStarted = false;

        // Tick cooldown
        if (cooldownTimer > 0.f) cooldownTimer = max(0.f, cooldownTimer - dt);

        const float dur = (attackAnimDurationSec > 0.f) ? attackAnimDurationSec : fallbackAnimDuration;

        // Start attack
        if (!isAttacking)
        {
            if (cooldownTimer <= 0.f && distanceToTarget <= startRange)
            {
                isAttacking = true;
                justStarted = true;

                attackTimer = 0.f;
                hitQueued = false;
                hitFired = false;

                cooldownTimer = cooldown; // start cooldown immediately
            }
            return;
        }

        if (distanceToTarget > breakRange)
        {
            isAttacking = false;
            return;
        }

        // During attack
        attackTimer += dt;

        float t = hitTimeNormalized;
        if (t < 0.f) t = 0.f;
        if (t > 1.f) t = 1.f;

        const float hitTime = dur * t;

        // Fire hit once
        if (!hitFired && attackTimer >= hitTime)
        {
            if (distanceToTarget <= hitRange)
                hitQueued = true;

            hitFired = true;
        }

        // End attack
        if (attackTimer >= dur)
            isAttacking = false;
    }

    bool IsAttacking() const { return isAttacking; }
    bool JustStarted() const { return justStarted; }

    // True exactly once at the hit moment of an attack
    bool PollHit()
    {
        if (!hitQueued) return false;
        hitQueued = false;
        return true;
    }

    void Reset()
    {
        isAttacking = false;
        justStarted = false;
        cooldownTimer = 0.f;
        attackTimer = 0.f;
        hitQueued = false;
        hitFired = false;
    }

private:
    bool  isAttacking = false;
    bool  justStarted = false;

    float cooldownTimer = 0.f;
    float attackTimer = 0.f;

    bool  hitQueued = false;
    bool  hitFired = false;

    float fallbackAnimDuration = 0.5f; // used if duration passed in is 0/invalid
};
