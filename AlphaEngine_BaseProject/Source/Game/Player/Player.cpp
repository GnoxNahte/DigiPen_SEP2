#include "Player.h"
#include "../Camera.h"
#include <iostream>
#include <limits>
#include "../../Utils/QuickGraphics.h"

Player::Player(MapGrid* map, float initialPosX, float initialPosY) :
    stats("Assets/config/player-stats.json"), sprite("Assets/Craftpix/Char_Robot.png"),
    facingDirection()
{
    this->map = map;

    position.x = initialPosX;
    position.y = initialPosY;

    isGrounded = true;
}

Player::~Player()
{
}

void Player::Update()
{
    UpdateInput();

    // Update velocity
    HorizontalMovement();
    VerticalMovement();

    // @todo: Ethan - Handle collision

    // Update position based on velocity
    AEVec2 displacement, nextPosition;
    // displacement = velcoity * dt
    AEVec2Scale(&displacement, &velocity, (f32)AEFrameRateControllerGetFrameTime());
    // nextPosition = position + displacement
    AEVec2Add(&nextPosition, &position, &displacement);
    map->HandleCollision(position, nextPosition, stats.playerSize);

    sprite.Update();

    // @todo - Delete, for debug only
    if (AEInputCheckCurr(AEVK_R))
        stats.LoadFileData();
}

void Player::Render()
{
    // Local scale. For flipping sprite's facing direction
    AEMtx33Scale(&transform, facingDirection.x > 0 ? 2.f : -2.f, 2.f);
    AEMtx33TransApply(
        &transform,
        &transform,
        position.x - (0.5f - sprite.metadata.pivot.x),
        position.y - (0.5f - sprite.metadata.pivot.y)
    );
    // Camera scale. Scales translation too.
    AEMtx33ScaleApply(&transform, &transform, Camera::scale, Camera::scale);
    AEGfxSetTransform(transform.m);

    // todo - delete, test only
    if (map->CheckCollision(position))
        AEGfxSetColorToMultiply(1, 0, 0, 1);

    sprite.Render();

    //AEVec2 
    QuickGraphics::DrawRect(position, stats.playerSize, 0xFFFF0000, AE_GFX_MDM_LINES_STRIP);

    // todo - delete, test only
    AEGfxSetColorToMultiply(1, 1, 1, 1);
}

void Player::UpdateInput()
{
    // Consider shift all keybinds to another file. Then maybe can allow custom keybinding 
    inputDirection.x = (f32)((AEInputCheckCurr(AEVK_RIGHT) || AEInputCheckCurr(AEVK_D))
        - (AEInputCheckCurr(AEVK_LEFT) || AEInputCheckCurr(AEVK_A)));
    inputDirection.y = (f32)((AEInputCheckCurr(AEVK_UP) || AEInputCheckCurr(AEVK_W))
        - (AEInputCheckCurr(AEVK_DOWN) || AEInputCheckCurr(AEVK_S)));

    isJumpHeld = AEInputCheckCurr(AEVK_SPACE);
    if (AEInputCheckTriggered(AEVK_SPACE))
        AEGetTime(&lastJumpPressed);

    if (inputDirection.x != 0 || inputDirection.y != 0)
        facingDirection = inputDirection;
}

void Player::HorizontalMovement()
{
    float dt = (float)AEFrameRateControllerGetFrameTime();

    /*if (AEInputCheckCurr(AEVK_LEFT) || AEInputCheckCurr(AEVK_A))
        --xInput;
    if (AEInputCheckCurr(AEVK_RIGHT) || AEInputCheckCurr(AEVK_D))
        ++xInput;*/

        // Slow player down when not pressing any buttons
    if (inputDirection.x == 0)
    {
        float velocityChange = stats.stopAcceleration * dt;
        // -velocityChange because stats.stopAcceleration < 0, so negate that
        if (fabsf(velocity.x) > -velocityChange)
            velocity.x += velocityChange * (velocity.x > 0.f ? 1.f : -1.f);
        // If velocity.x < -velocityChange, set to 0 to make sure it won't overshoot
        else
            velocity.x = 0.f;
    }
    else
    {
        // If moving in the same direction
        if (inputDirection.x * velocity.x >= 0)
        {
            velocity.x += stats.moveAcceleration * inputDirection.x * dt;

            float maxSpeed = isGrounded ? stats.maxSpeed : stats.airStrafeMaxSpeed;
            velocity.x = AEClamp(velocity.x, -maxSpeed, maxSpeed);
        }
        // Moving in opposite direction
        else
        {
            float acceleration = isGrounded ? stats.turnAcceleration : stats.inAirTurnAcceleration;
            velocity.x -= acceleration * inputDirection.x * dt;
        }
    }
}


void Player::VerticalMovement()
{
    HandleLanding();
    HandleGravity();
    HandleJump();
}

void Player::HandleLanding()
{
    if (!isGrounded || velocity.y > 0.f)
        return;

    // @todo: (Ethan) - One way platform?
    if (inputDirection.y < 0)
    {

    }
    // Land on ground
    else
    {
        // @todo: (Ethan) - Play sound
        if (!isGrounded)

            lastJumpTime = std::numeric_limits<f64>::lowest();;
        AEGetTime(&lastGroundedTime);
    }
}

void Player::HandleGravity()
{
    float dt = (float)AEFrameRateControllerGetFrameTime();
    isGrounded = false;

    // If moving up
    if (velocity.y > 0.f)
    {
        // @todo: (Ethan) - Hit handle ceiling

        float velocityChange = stats.gravity * dt;
        f64 currTime = AEGetTime(nullptr);
        if (!isJumpHeld && (currTime - lastJumpTime) > stats.minJumpTime)
            velocityChange *= stats.gravityMultiplierWhenRelease;

        velocity.y += velocityChange;
    }
    // If on ground
    // @todo: (Ethan) - Collision ground check
    else if (position.y <= 0.f)
    {
        isGrounded = true;
        velocity.y = 0.f;

        position.y = 0.f; // TODO - remove this
    }
    // Else playing falling
    else
    {
        velocity.y = max(velocity.y + stats.fallingGravity * dt, stats.maxFallVelocity);
    }
}

void Player::HandleJump()
{
    f64 currTime = AEGetTime(nullptr);

    bool isJumpBufferActive = (currTime - lastJumpPressed) < stats.jumpBuffer;
    bool isCoyoteTimeActive = (currTime - lastGroundedTime) < stats.coyoteTime;
    bool ifJump = isJumpBufferActive && isCoyoteTimeActive;

    if (ifJump)
    {
        constexpr f64 lowestFloat = std::numeric_limits<f64>::lowest();
        velocity.y = stats.jumpVelocity;
        lastJumpPressed = lowestFloat; // Prevent jump buffer from triggering again
        lastGroundedTime = lowestFloat;
        lastJumpTime = currTime;
        ifReleaseJumpAfterJumping = false;
    }
}
