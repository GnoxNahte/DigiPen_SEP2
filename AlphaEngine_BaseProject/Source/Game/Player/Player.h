#pragma once

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
    bool isJumpHeld = false;
    f64 lastJumpPressed = -1.f;
    bool ifReleaseJumpAfterJumping = true;

    // === Movement data ===
    AEVec2 facingDirection;
    f64 lastJumpTime = -1.f;
    f64 lastGroundedTime = -1.f;
    
    // === Collision ===
    bool isGroundCollided = false;
    bool isCeilingCollided = false;
    bool isLeftWallCollided = false;
    bool isRightWallCollided = false;

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

