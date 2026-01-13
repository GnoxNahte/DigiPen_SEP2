#pragma once

#include <string>
#include "AEEngine.h"
#include "PlayerStats.h"
#include "../../Utils/Sprite.h"
#include "../Environment/MapGrid.h"
#include "../../Utils/Box.h"

/**
 * @brief Controllable player class
 */
class Player
{
public:

    // === Movement ===
    AEVec2 position;
    AEVec2 velocity;

    Player(MapGrid* map, float initialPosX, float initialPosY);
    ~Player();
    void Update();
    void Render();
private:
    PlayerStats stats;
    Sprite sprite;

    AEMtx33 transform;

    // === Player Input ===
    AEVec2 inputDirection;
    bool isJumpHeld;
    f64 lastJumpPressed;
    bool ifReleaseJumpAfterJumping;

    // === Movement data ===
    AEVec2 facingDirection;
    bool isGrounded;
    f64 lastJumpTime;
    f64 lastGroundedTime;
    
    // === Collision ===
    bool isGroundCollided;
    bool isCeilingCollided;
    bool isLeftWallCollided;
    bool isRightWallCollided;

    // === References to other systems ===
    MapGrid* map;

    void UpdateInput();

    void HorizontalMovement();
    void VerticalMovement();
    void HandleLanding();
    void HandleGravity();
    void HandleJump();
    void PerformJump();

    void RenderDebugCollider(Box& box);
};

