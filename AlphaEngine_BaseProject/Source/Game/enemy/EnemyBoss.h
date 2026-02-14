#pragma once
#include "../../Utils/Sprite.h" 
#include "EnemyAttack.h"
#include <AEVec2.h>



class EnemyBoss
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

    //raise to start chasing player
    float aggroRange = 10.0f;


    //for gamescene to use to apply damage later
    bool PollAttackHit() { return attack.PollHit(); }

  






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
        SPELL2 = 7
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
};
#pragma once
