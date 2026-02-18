#pragma once

#include "AEEngine.h"
#include "PlayerStats.h"
#include "../../Utils/Sprite.h"
#include "../../Utils/Box.h"
#include "../../Utils/ParticleSystem.h"
#include "../Environment/MapGrid.h"
#include "../../Editor/EditorUtils.h"

/**
 * @brief Controllable player class
 */
class Player : Inspectable
{
public:
    enum AnimState
    {
        IDLE_NO_SWORD,
        CROUCH,      
        RUN_NO_SWORD,
        SOMERSAULT,  
        FALLING,     
        SLIDING,     
        IDLE_W_SWORD,
        ATTACK_1,    
        ATTACK_2,    
        ATTACK_3,    
        ATTACK_END = ATTACK_3,
        HURT,        
        DEATH,       
        SWORD_DRAW,  
        SWORD_SHEATH,
        WALL_SLIDE,  
        WALL_CLIMB,  
        // @todo - Split into air attack and attack down (smash)?
        AIR_ATTACK_1,
        AIR_ATTACK_2,
        AIR_ATTACK_3,
        AIR_ATTACK_END = AIR_ATTACK_3,
        RUN_W_SWORD, 

        ANIM_COUNT
    };

    Player(MapGrid* map);
    ~Player();
    void Update();
    void Render();
    void Reset(const AEVec2& initialPos);

    void TakeDamage(int dmg, const AEVec2& hitOrigin);

    void DrawInspector() override;
    bool CheckIfClicked(const AEVec2& mousePos) override;
    
    // === Getters ===
    const AEVec2& GetPosition() const;
    int GetHealth() const;
    const PlayerStats& GetStats() const;
    bool IsFacingRight() const;

private:
    PlayerStats stats;
    Sprite sprite;

    AEMtx33 transform;
    ParticleSystem particleSystem;

    // === Player Input ===
    AEVec2 inputDirection;
    bool isJumpHeld = false;
    f64 lastJumpPressed = -1.f;
    bool ifReleaseJumpAfterJumping = true;
    f64 lastAttackHeld = -1.f;

    // === Movement data ===
    AEVec2 position;
    AEVec2 velocity;
    AEVec2 facingDirection;
    f64 lastJumpTime = -1.f;
    f64 lastGroundedTime = -1.f;
    f64 dashStartTime = -1.f;
    
    // === Collision ===
    bool isGroundCollided = false;
    bool isCeilingCollided = false;
    bool isLeftWallCollided = false;
    bool isRightWallCollided = false;

    // === Combat ===
    int health;

    // === References to other systems ===
    MapGrid* map;

    // ===== Helper Functions =====
    void UpdateInput();
    void UpdateTriggerColliders();

    // === Movement update ===
    void HorizontalMovement();
    void VerticalMovement();
    void HandleLanding();
    void HandleGravity();
    void HandleJump();
    void PerformJump();

    bool IsAnimGroundAttack();
    bool IsAnimAirAttack();
    bool IsAttacking();
    void UpdateAttacks();
    void OnAttackAnimEnd(int spriteStateIndex);

    void UpdateTrails(); // Might remove, now just for testing
    void UpdateAnimation();

    void RenderDebugCollider(Box& box);
};

