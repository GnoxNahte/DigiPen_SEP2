#pragma once
#include <string>
#include <array>
#include "AEEngine.h"
#include "../../Utils/Box.h"

struct AttackStats
{
    int damage;
    float knockbackForce;
    Box collider;
};

/**
 * @brief   Contains the Player stats
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
    float gravityMultiplierWhenRelease;
    float coyoteTime;
    float jumpBuffer;
    float wallJumpHorizontalVelocity;
    float wallJumpHorizontalVelocityTowardsWall;

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

	// === Combat stats ===
	int maxHealth;
    float attackBuffer;

    std::array<AttackStats, 3> groundAttacks;
    std::array<AttackStats, 3> airAttacks;

    const std::string file;

    /**
     * @brief       Loads the player stats from a file
     * @param file  File to load the stats from
     */
    PlayerStats(std::string file);

    void LoadFileData();
    void OnDataChanged();

    // Player will call this instead of editor
    void DrawInspector();
private:
    void CalculateDerivedVariables();
};

