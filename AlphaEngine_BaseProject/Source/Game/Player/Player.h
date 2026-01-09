#pragma once

#include <string>
#include "AEEngine.h"
#include "PlayerStats.h"
#include "../../Utils/Sprite.h"

/**
 * @brief Controllable player class
 */
class Player
{
public:
    // === Movement ===
    AEVec2 position;
    AEVec2 velocity;

    Player(float initialPosX, float initialPosY);
    ~Player();
    void Update();
    void Render();
private:
    PlayerStats stats;
    Sprite sprite;

    float playerHeight;
    AEMtx33 transform;

    // === Player Input ===
    AEVec2 inputDirection;
    bool isJumpHeld;
    f64 lastJumpPressed;
    bool ifReleaseJumpAfterJumping;

    AEVec2 facingDirection;
    bool isGrounded;
    f64 lastJumpTime;
    f64 lastGroundedTime;

    void UpdateInput();

    void HorizontalMovement();
    void VerticalMovement();
    void HandleLanding();
    void HandleGravity();
    void HandleJump();
};

