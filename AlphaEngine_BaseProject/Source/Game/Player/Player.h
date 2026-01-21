#pragma once

#include "AEEngine.h"
#include "PlayerStats.h"
#include "../../Utils/Sprite.h"
#include "../../Utils/Box.h"
#include "../../Utils/ParticleSystem.h"
#include "../Environment/MapGrid.h"

/**
 * @brief Controllable player class
 */
class Player
{
public:
    enum AnimState
    {
        RUN_ATTACK,
        RUN,
        IDLE,
        JUMP_START,
        JUMP_FALL,
        JUMP_LAND,
        ATTACK,
        DEATH,
        AIMING,
        HURT
    };

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
    ParticleSystem particleSystem;

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

    void UpdateAnimation();

    void RenderDebugCollider(Box& box);
};

