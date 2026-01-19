#pragma once
#include "AEEngine.h"
#include "../../Utils/Sprite.h" 
#include "../../Utils/QuickGraphics.h"
#include "EnemyAttack.h"



class EnemyA
{
public:
    EnemyA(float initialPosX = 0.f, float initialPosY = 0.f);
    ~EnemyA();

    void Update(const AEVec2& playerPos);
    void Render();

    AEVec2 position{ 0.f, 0.f }; 


    bool isDead = false;
    bool isAttacking = false;
    bool isGrounded = true; 
    bool chasing = false;

    //raise to start chasing player
    float aggroRange = 5.0f;


    //for gamescene to use to apply damage later
    bool PollAttackHit() { return attack.PollHit(); }





private:

    enum AnimState
    {
        IDLE1 = 0,
        RUN = 1,
        ATTACK = 2,
        JUMP = 3,
        DEATH = 4
    };
    void UpdateAnimation();
    AEVec2 velocity{ 0.f, 0.f };
    Sprite sprite;
    AEVec2 facingDirection{ 1.f, 0.f };  


    float width{ 0.8f };
    float height{ 0.8f };

    float moveSpeed{ 2.2f };
    float stopDistance{ 0.2f };


    EnemyAttack attack;

   

    // Debug / collider size (use AEVec2 like Player)
    AEVec2 size{ 0.8f, 0.8f };
    bool debugDraw{ true };
};
#pragma once