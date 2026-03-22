#pragma once

#include "AEEngine.h"
#include "PlayerStats.h"
#include "../../Utils/Sprite.h"
#include "../../Utils/Box.h"
#include "../../Utils/ParticleSystem.h"
#include "../../Utils/Event/EventSystem.h"
#include "../Environment/MapGrid.h"
#include "../../Editor/EditorUtils.h"
#include "../enemy/EnemyManager.h"
#include "../enemy/IDamageable.h"
#include "../BuffCards.h"

/**
 * @brief Controllable player class
 */
class Player : public Inspectable, IDamageable
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
        DEATH_LOOP,       
        SWORD_DRAW,  
        SWORD_SHEATH,
        WALL_SLIDE,  
        WALL_CLIMB,  
        // @todo - Split into air attack and attack down (smash)?
        AIR_ATTACK_1,
        AIR_ATTACK_2,
        AIR_ATTACK_SMASH,
        AIR_ATTACK_END = AIR_ATTACK_SMASH,
        RUN_W_SWORD, 

        ANIM_COUNT
    };

    Player(MapGrid* map, EnemyManager* enemyManager);
    ~Player();
    void Update();
    void Render();
    void Reset(const AEVec2& initialPos);

    // === Inspectable ===
    void DrawInspector() override;
    bool CheckIfClicked(const AEVec2& mousePos) override;

    // === IDamageable ===
    const AEVec2& GetHurtboxPos() const override;
    const AEVec2& GetHurtboxSize() const override;
    bool IsDead() const override;
    bool TryTakeDamage(int dmg, const AEVec2& hitOrigin, DAMAGE_TYPE type = DAMAGE_TYPE_ENEMY_ATTACK) override;

    // === Getters ===
    const AEVec2&       GetPosition() const;
    const PlayerStats&  GetStats()    const;
    float   GetDashCooldownPercentage() const;
    int     GetHealth()     const;
    int     GetMaxHealth()     const;
    bool    GetIsFacingRight() const;

    AnimState GetAnimState() const;

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
    bool isFacingRight;
    f64 lastJumpTime = -1.f;
    f64 lastGroundedTime = -1.f;
    f64 dashStartTime = -1.f;
    
    // === Collision ===
    bool isGroundCollided = false;
    bool isCeilingCollided = false;
    bool isLeftWallCollided = false;
    bool isRightWallCollided = false;

    // Keeps track of enemies that the current attack has hit
    std::vector<IDamageable*> attackedEnemies;

    // === Combat ===
    int maxHealth;
    int health;
    bool hasAppliedRecoil; // For current attack
    f64 lastDamagedTime;
    f64 lastAttackEndTime;
    AnimState lastAttackCombo;
    float slamStartHeight;

    // === Buffs ===
    float buff_MoveSpeedMulti;
    float buff_DmgReduction;
    float buff_TrapDmgReduction;
    float buff_critChance;
    float buff_critDmgMulti;
    float buff_DmgMultiLowHP;
    float buff_DashCooldownMulti;
    
    EventId buffEventId;

    // === References to other systems ===
    MapGrid* map;
    EnemyManager* enemyManager;

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
    void UpdateCollisions(const AEVec2& nextPosition);

    bool IsDashing();
    bool IsAnimGroundAttack();
    bool IsAnimAirAttack();
    bool IsAttacking();
    bool IsInvincible();
    void SetAttack(AnimState toState);
    void AttackDamageable(IDamageable& damageable, const AttackStats& attack, bool isGroundAttack);
    void UpdateAttacks();
    void OnAttackAnimEnd(int spriteStateIndex);
    IDamageable* IfCollideEnemy(const Box& collider);
    float GetSlamAttackScale();

    void UpdateTrails(); // Might remove, now just for testing
    void UpdateAnimation();

    void RenderDebugCollider(Box& box);

    void OnBuffSelected(const BuffSelectedEvent& ev);
};

// ===== Events =====
struct PlayerDeathEvent
{
    const Player& player;
};
