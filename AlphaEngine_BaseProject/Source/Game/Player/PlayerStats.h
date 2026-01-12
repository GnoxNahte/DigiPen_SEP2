#pragma once
#include <string>
#include "AEEngine.h"

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

