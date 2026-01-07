#pragma once
#include <string>
#include "AEEngine.h"

/**
 * @brief   Contains the Player stats
 */
class PlayerStats
{
public:
    //// === Horizontal Movement ===
    //float maxSpeed;
    //float airStrafeMaxSpeed;

    //float maxSpeedTime;
    //float stopTime;
    //float turnTime;
    //float inAirTurnTime;
    //float moveAcceleration;
    //float stopAcceleration;
    //float turnAcceleration;
    //float inAirTurnAcceleration;

    //float dashCooldown;
    //float dashTime;

    //// ===== Gravity =====
    //float maxFallVelocity;
    //float gravity;
    //float fallingGravity;
    //float wallSlideMaxSpeedTime;

    //float wallSlideMaxSpeed;
    //float wallSlideAcceleration;

    //// ===== Jumping =====
    //float maxJumpHeight;
    //float minJumpHeight;
    //float minJumpTime;
    //float timeToMaxHeight;
    //float timeToGround;
    //float jumpVelocity;
    //float gravityMultiplierWhenRelease;
    //float coyoteTime;
    //float jumpBuffer;
    //float wallJumpHorizontalVelocity;
    //float wallJumpHorizontalVelocityTowardsWall;

    //// ===== Others =====

    //float invincibilityDuration;

    /**
     * @brief       Loads the player stats from a file
     * @param file  File to load the stats from
     */
    PlayerStats(std::string file);

private:
    // @todo: Ethan - File watcher
    void WatchFile(std::string file);
};

