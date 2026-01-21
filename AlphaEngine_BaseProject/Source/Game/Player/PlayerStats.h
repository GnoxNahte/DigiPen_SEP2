#pragma once
#include <string>
#include "AEEngine.h"
#include "../../Utils/Box.h"

/**
 * @brief   Contains the Player stats
 */
class PlayerStats
{
public:
    // ===== Stats from JSON =====
    // === Horizontal Movement ===
    float maxSpeed;
    float airStrafeMaxSpeed;

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

    const std::string file;

	// === Player's HP and attacks ===
	int playerMaxHP;
	int playerStartingHP;
	int playerAttackDamage;
	float playerAttackRange;
	float playerAttackCooldown;
	float playerAttackKnockbackForce;
    

    /**
     * @brief       Loads the player stats from a file
     * @param file  File to load the stats from
     */
    PlayerStats(std::string file);

    void LoadFileData();
private:
    // @todo: Ethan - File watcher
    void WatchFile();
};

