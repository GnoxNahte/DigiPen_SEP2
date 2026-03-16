#pragma once
#include <string>
#include <array>
#include "AEEngine.h"
#include "../../Utils/Box.h"

struct AttackStats
{
    int damage;
    float recoilSpeed;
    Box collider;
};

/**
 * @brief   Contains the Player stats
 *          To add a property, need to add it in all these places
 *          1. Add variable in this struct
 *          2. LoadFileData()
 *          3. SaveFileData()
 *          4. DrawInspector()
 *          5. Add it in the .json file. Not handling if it can't find it. Don't want to silently fail
 *          6. + Wherever the variable is used (e.g. Player.h)
 *          
 *          Lots of steps and alot of repeated code 
 *          but since C++ doesn't have reflection, not sure how else to do.
 *          Tried LIONant's xproperty and RTTR but can't get them to compile in the project...
 */
struct PlayerStats
{
    // ===== Stats from JSON =====
    // === Horizontal Movement ===
    float maxSpeed;
    float airStrafeMaxSpeed;
    float attackMaxSpeedMultiplier;

    float maxSpeedTime;
    float stopTime;
    float turnTime;
    float inAirTurnTime;

    AEVec2 dashSpeed;
    float dashCooldown;
    float dashTime;

    // ===== Vertical movement =====
    float maxFallVelocity;
    float wallSlideMaxSpeedTime;

    float wallSlideMaxSpeed;

    // ===== Jumping =====
    float maxJumpHeight;
    float minJumpHeight;
    float timeToMaxHeight;
    float timeToGround;
    float gravityMultiplierWhenRelease; // Multiple gravity if jump button is released
    float coyoteTime;                   // Allows player to jump for some time after leaving the platform
    float jumpBuffer;                   // Allows the player to jump if they press and release 'Jump' before reaching the ground
    float wallJumpHorizontalVelocity;   // Default Wall jump horizontal velocity 
    float wallJumpHorizontalVelocityTowardsWall; //Wall jump horizontal velocity when horizontal input is towards wall

    // === Colliders ===
    Box groundChecker;
    Box ceilingChecker;
    Box leftWallChecker;
    Box rightWallChecker;

    // === Others ===
    AEVec2 playerSize;

    // ===== Stats derived from JSON data =====
    // === Horizontal movement ===
    float moveAcceleration;
    float stopAcceleration;
    float turnAcceleration;
    float inAirTurnAcceleration;

    // === Vertical movement ===
    float gravity;
    float fallingGravity;

    float wallSlideAcceleration;

    float jumpVelocity;
    float minJumpTime;

	// ===== Combat stats =====
	int maxHealth;
    float invincibleTime;       // After taking damage
    float knockbackAmt;         // Knockback when taking damage
    float maxKnockbackDmg;      // At max dmg, apply full knockback
    float attackBuffer;         // Input attack buffer
    float attackComboBuffer;    // Input attack buffer for combos
    float slamAttackFallSpeed;  
    float slamAttackMaxHeight;  // Applies max damage at max height

    std::array<AttackStats, 3> groundAttacks;
    std::array<AttackStats, 3> airAttacks; // Currently only using last attack as ground slam

    const std::string file;

    /**
     * @brief       Loads the player stats from a file
     * @param file  File to load the stats from
     */
    PlayerStats(std::string file);

    void LoadFileData();
    void SaveFileData();
    void OnDataChanged();

    // Player will call this instead of editor
    void DrawInspector();
private:
    void CalculateDerivedVariables();
};

