#pragma once
#include "../../Utils/Sprite.h" 
#include "EnemyAttack.h"
#include <AEVec2.h>
#include "IDamageable.h"
#include "../../Editor/EditorUtils.h"



class EnemyBoss : public IDamageable, Inspectable
{
public:
    EnemyBoss(float initialPosX = 0.f, float initialPosY = 0.f);
    ~EnemyBoss();

    void Update(const AEVec2& playerPos, bool playerFacingRight);

    void Render();
    
    AEVec2 position{ 0.f, 0.f };


    bool isDead = false;
    bool isAttacking = false;
    bool isGrounded = true;
    bool chasing = false;
    int attackDamage = 10;

    //raise to start chasing player
    float aggroRange = 10.0f;

    bool IsDead() const override { return isDead; }
    //for gamescene to use to apply damage later
    bool PollAttackHit() { return !isDead && attack.PollHit(); }
    // returns number of special projectiles that hit the player this frame
    int ConsumeSpecialHits(const AEVec2& playerPos, const AEVec2& playerSize);
    // Single hurtbox for now (same as your debug rect in Render()).
    AEVec2 GetHurtboxPos() const { return position; }
    AEVec2 GetHurtboxSize() const { return size; }
    int GetHP() const { return hp; }
    int GetMaxHP() const { return maxHP; }

	//IMGUI inspector
    void DrawInspector() override;
    bool CheckIfClicked(const AEVec2& mousePos) override;

    


    int   GetAttackDamage() const { return attackDamage; }
    bool IsInvulnerable() const { return invulnTimer > 0.f; }
    bool TryTakeDamage(int dmg, int attackInstanceId = -1) override;
    // Convenience: checks overlap vs boss hurtbox first, then applies damage.
    bool TryTakeDamageFromHitbox(const AEVec2& hitPos, const AEVec2& hitSize,
        int dmg, int attackInstanceId = -1);



  






private:

    enum AnimState
    {
        IDLE = 0,
        RUN = 1,
        ATTACK = 2,
        HURT = 3,
        TELEPORT = 4,
        SPELLCAST = 5,
        SPELL1 = 6,
        DEATH = 7
    };
    void UpdateAnimation();
    AEVec2 velocity{ 0.f, 0.f };
    Sprite sprite;
    Sprite specialAttackVfx;   // extra sprite used only for special attack VFX

    AEVec2 facingDirection{ 1.f, 0.f };


    float width{ 0.8f };
    float height{ 0.8f };

    float moveSpeed{ 2.2f };
    float stopDistance{ 0.2f };

       // --- Teleport control ---
    float teleportCooldownTimer{ 0.0f };   // counts up until teleport triggers
    float teleportInterval{ 2.0f };        // teleport every 2s (tune)
    bool  teleportActive{ false };         // currently playing TELEPORT anim
    float teleportTimer{ 0.0f };           // time since TELEPORT started
    bool  teleportMoved{ false };          // have we snapped behind player yet?

    float teleportBehindOffset{ 0.9f };    // how far behind player (tune)
    float teleportMoveNormalized{ 0.5f };  // when to snap (0..1 of teleport anim)



    EnemyAttack attack;

    // --- Special attack sequence control ---
    float SpecialElapsed{ 0.0f };           // cooldown timer (seconds)
    bool specialUnlocked{ false };          // becomes true after first normal attack
    bool specialBurstActive{ false };       // currently spawning the 5 specials
    int  specialSpawnsRemaining{ 0 };       // how many left in this burst
    float specialSpawnTimer{ 0.0f };        // time until next spawn in burst


    // Debug / collider size (use AEVec2 like Player)
    AEVec2 size{ 0.8f, 0.8f };
    bool debugDraw{ true };

    // --- Health / damage ---
    int   maxHP{ 10 };
    int   hp{ 10 };

    // Hurt lock (like regular Enemy): keeps HURT animation visible + blocks re-hits.
    float hurtTimeLeft{ 0.0f };
    float minHurtDuration{ 0.30f };

    float invulnTimer{ 0.0f };
    float invulnDuration{ 0.20f };

    // Used only if you pass attackInstanceId >= 0 (optional)
    int   lastHitAttackId{ -1 };

    float deathTimeLeft{ 0.0f };

	bool hideAfterDeath{ false };

    // --- Boss UI (screen-space healthbar) ---
    float hpBarShown{ 1.0f };          // 0..1 smoothed display
    float hpBarDrainPerSec{ 0.60f };   // smaller = slower drain (tune)
    bool  showHealthbar{ true };       // optional toggle
    void RenderHealthbar() const;
};

