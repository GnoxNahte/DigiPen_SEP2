#pragma once
#include <string>
#include "AEEngine.h"

class PlayerStats
{
public:
	// === Horizontal Movement ===
    float maxSpeed;
    float airStrafeMaxSpeed;

    float maxSpeedTime;
    float stopTime;
    float turnTime;
    float inAirTurnTime;
    float moveAcceleration;
    float stopAcceleration;
    float turnAcceleration;
    float inAirTurnAcceleration;

    float dashCooldown;
    float dashTime;

    float spinHorizontalSpeed;
    float spinVerticalSpeed;

    // ===== Gravity =====

    float maxFallVelocity;
    float gravity;
    float fallingGravity;
    float wallSlideMaxSpeedTime;
    
    float wallSlideMaxSpeed;
    float wallSlideAcceleration;

    // ===== Jumping =====
    float maxJumpHeight;
    float minJumpHeight;
    float minJumpTime;
    float timeToMaxHeight;
    float timeToGround;
    float jumpVelocity;
    float gravityMultiplierWhenRelease;
    float coyoteTime;
    float jumpBuffer;
    float wallJumpHorizontalVelocity;
    float wallJumpHorizontalVelocityTowardsWall;

    AEVec2 spinHitVelocity;
    float minKnockbackVerticalSpeed;

    // ===== Others =====
    
    float invincibilityDuration;

    PlayerStats(std::string file);
};

class Player
{
    
};

