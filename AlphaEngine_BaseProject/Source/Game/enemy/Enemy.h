#pragma once

#include "../../Utils/Sprite.h"
#include "EnemyAttack.h"
#include <AEVec2.h>
#include "IDamageable.h"
#include "../../Editor/EditorUtils.h"


class Enemy : public IDamageable, Inspectable
{
public:
    // Presets to replace EnemyA / EnemyB
    enum class Preset
    {
        Druid,
        Skeleton
    };

    struct Config
    {
        const char* spritePath = nullptr;

        int maxHp = 10;   // basic life system
        int attackDamage = 1;
        bool hideAfterDeath = false;


        // Render
        float renderScale = 2.f;

        // Movement / AI
        float moveSpeed = 2.0f;
        float aggroRange = 5.0f;
        float leashRange = 8.0f;

        // Animation selection
        float runVelThreshold = 0.1f; // when to play RUN instead of IDLE

        // Attack tuning
        float attackStartRange = 1.1f;
        float attackHitRange = 1.5f;
        float attackCooldown = 0.8f;
        float attackHitTimeNormalized = 0.5f; // 0..1
        float attackBreakRange = 1.2f;

        // Row indices in the sprite meta (IMPORTANT if your meta order differs)
        int animAttack = 0;
        int animRun = 2;
        int animIdle = 3;
        int animHurt = 4;
        int animDeath = 1;
    };

public:
    Enemy(Preset preset = Preset::Druid, float initialPosX = 0.f, float initialPosY = 0.f);
    Enemy(const Config& cfg, float initialPosX, float initialPosY);
    ~Enemy() = default;

    AEVec2 GetHurtboxPos()  const override { return GetPosition(); }
    AEVec2 GetHurtboxSize() const override { return GetSize(); }

    void DrawInspector() override;
    bool CheckIfClicked(const AEVec2& mousePos) override;

    void Update(const AEVec2& playerPos);
    void Render();

    // For GameScene to apply damage later
    bool PollAttackHit() { return !dead && attack.PollHit(); }
    

    //For enemy life system
    bool IsDead() const { return dead; }
    int  GetHP() const { return hp; }

    // Returns true if damage was actually applied.
    bool TryTakeDamage(int dmg, const AEVec2& hitOrigin) override;

 
    //void ApplyDamage(int dmg);


    // Useful getters for combat / debugging
    AEVec2 GetPosition() const { return position; }
    AEVec2 GetSize() const { return size; }
    bool   IsChasing() const { return chasing; }
    bool   IsReturningHome() const { return returningHome; }

    float GetAttackHitRange() const { return attack.hitRange; }   // mid/close range
    int   GetAttackDamage() const { return cfg.attackDamage; }                  


    // Optional knobs
    void SetDebugDraw(bool on) { debugDraw = on; }

private:
    void UpdateAnimation();
    static Config MakePreset(Preset preset);
    static float GetAnimDurationSec(const Sprite& sprite, int stateIndex);

private:
    Config cfg;

    Sprite sprite;
    EnemyAttack attack;

    AEVec2 position{ 0.f, 0.f };
    AEVec2 homePos{ 0.f, 0.f };
    AEVec2 velocity{ 0.f, 0.f };
    AEVec2 size{ 0.8f, 0.8f };
    AEVec2 facingDirection{ 1.f, 0.f };

    bool chasing{ false };
    bool returningHome{ false };
    bool debugDraw{ true };

    int  hp{ 1 };
    bool dead{ false };

    // Hurt lock: keeps the hurt animation visible long enough to notice
    float hurtTimeLeft{ 1.0f };
    float deathTimeLeft{ 0.5f };
    bool hidden = false;
  


};
