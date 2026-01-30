#include "Player.h"
#include "../Camera.h"
#include <iostream>
#include <limits>
#include "../../Utils/QuickGraphics.h"
#include "../../Utils/AEExtras.h"
#include <imgui.h>
#include "../../Editor/ImGuiHelper.h"

Player::Player(MapGrid* map) :
    stats("Assets/config/player-stats.json"), 
    sprite("Assets/Art/rvros/Adventurer.png"),
    facingDirection{},
    inputDirection{},
    transform{},
    velocity{},
    particleSystem{ 50, {} }
{
    std::cout << "Construct player";
    Reset(AEVec2{ 2, 4 });

    this->map = map;

    health = stats.maxHealth;

    particleSystem.Init();
    particleSystem.emitter.lifetimeRange.x = 0.1f;
    particleSystem.emitter.lifetimeRange.y = 0.3f;
}

Player::~Player()
{
}

void Player::Update()
{
    UpdateInput();
    UpdateTriggerColliders();

    // Update velocity
    HorizontalMovement();
    VerticalMovement();

    UpdateAttacks();

    // Update position based on velocity
    AEVec2 displacement, nextPosition;
    AEVec2Scale(&displacement, &velocity, (f32)AEFrameRateControllerGetFrameTime());
    AEVec2Add(&nextPosition, &position, &displacement);
    map->HandleBoxCollision(position, velocity, nextPosition, stats.playerSize);

    UpdateAnimation();
    UpdateTrails();

    // @todo - Delete, for debug only
    if (AEInputCheckCurr(AEVK_R))
        stats.LoadFileData();
}

void Player::Render()
{
    particleSystem.Render();

    // Local scale. For flipping sprite's facing direction
    bool ifFaceRight = (velocity.x != 0.f) ? (velocity.x > 0) : (facingDirection.x > 0);
    // Multiply height by 0.74f because sprite aspect ratio isn't a square
    AEMtx33Scale(&transform, ifFaceRight ? 2.f : -2.f, 2.f * 0.74f);
    AEMtx33TransApply(
        &transform,
        &transform,
        position.x - (0.5f - sprite.metadata.pivot.x),
        position.y + (0.5f - sprite.metadata.pivot.y)
    );
    // Camera scale. Scales translation too.
    AEMtx33ScaleApply(&transform, &transform, Camera::scale, Camera::scale);
    AEGfxSetTransform(transform.m);

    sprite.Render();

    if (AEInputCheckCurr(AEVK_LCTRL))
    {
        RenderDebugCollider(stats.groundChecker);
        RenderDebugCollider(stats.ceilingChecker);
        RenderDebugCollider(stats.leftWallChecker);
        RenderDebugCollider(stats.rightWallChecker);
        QuickGraphics::DrawRect(position, stats.playerSize, 0xFFFF0000, AE_GFX_MDM_LINES_STRIP);
    }
}

void Player::Reset(const AEVec2& initialPos)
{
    position = initialPos;
    AEVec2Zero(&velocity);
    AEMtx33Identity(&transform);
    
    AEVec2Set(&inputDirection, 1.f, 0.f);
    isJumpHeld = false;
    lastJumpPressed = -1.f;
    ifReleaseJumpAfterJumping = true;

    facingDirection = inputDirection;
    lastJumpTime = -1.f;
    lastGroundedTime = -1.f;

    isGroundCollided = false;
    isCeilingCollided = false;
    isLeftWallCollided = false;
    isRightWallCollided = false;
}

void Player::TakeDamage(int dmg)
{
    if (dmg <= 0) 
        return;

    health = max(health - dmg, 0);

#if _DEBUG
    std::cout << "[Player] Damage: " << dmg
        << " HP=" << health << "/" << stats.maxHealth << "\n";
#endif

    // if animiation and sound is needed for getting hurt
    // sprite.SetState(AnimState::HURT);
}

const AEVec2& Player::GetPosition() const
{
    return position;
}

int Player::GetHealth() const
{
    return health;
}

const PlayerStats& Player::GetStats() const
{
    return stats;
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

    if (AEInputCheckCurr(AEVK_X))
        AEGetTime(&lastAttackHeld);
}

void Player::UpdateTriggerColliders()
{
    AEVec2 tmpPosition;

    AEVec2Add(&tmpPosition, &position, &stats.groundChecker.position);
    isGroundCollided = map->CheckBoxCollision(tmpPosition, stats.groundChecker.size);

    AEVec2Add(&tmpPosition, &position, &stats.ceilingChecker.position);
    isCeilingCollided = map->CheckBoxCollision(tmpPosition, stats.ceilingChecker.size);

    AEVec2Add(&tmpPosition, &position, &stats.leftWallChecker.position);
    isLeftWallCollided = map->CheckBoxCollision(tmpPosition, stats.leftWallChecker.size);

    AEVec2Add(&tmpPosition, &position, &stats.rightWallChecker.position);
    isRightWallCollided = map->CheckBoxCollision(tmpPosition, stats.rightWallChecker.size);
}

void Player::HorizontalMovement()
{
    float dt = (float)AEFrameRateControllerGetFrameTime();

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

            float maxSpeed = isGroundCollided ? stats.maxSpeed : stats.airStrafeMaxSpeed;
            if (IsAttacking())
                maxSpeed *= stats.attackMaxSpeedMultiplier;
            velocity.x = AEClamp(velocity.x, -maxSpeed, maxSpeed);
        }
        // Moving in opposite direction
        else
        {
            float acceleration = isGroundCollided ? stats.turnAcceleration : stats.inAirTurnAcceleration;
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
    if (!isGroundCollided || velocity.y > 0.f)
        return;

    // @todo: (Ethan) - One way platform?
    if (inputDirection.y < 0)
    {

    }
    // Land on ground
    else
    {
        // @todo: (Ethan) - Play sound
        if (!isGroundCollided)
            lastJumpTime = std::numeric_limits<f64>::lowest();
        
        AEGetTime(&lastGroundedTime);
    }
}

void Player::HandleGravity()
{
    float dt = (float)AEFrameRateControllerGetFrameTime();

    // If moving up
    if (velocity.y > 0.f)
    {
        if (isCeilingCollided)
            velocity.y = 0;
        else
        {
            float velocityChange = stats.gravity * dt;
            if (!isJumpHeld && (AEGetTime(nullptr) - lastJumpTime) > stats.minJumpTime)
                velocityChange *= stats.gravityMultiplierWhenRelease;

            velocity.y += velocityChange;
        }
    }
    // If on ground
    else if (isGroundCollided)
    {
        velocity.y = 0.f;
    }
    // Else, falling
    else
    {
        float acceleration, maxFallSpeed;
        // Wall slide
        if (isLeftWallCollided || isRightWallCollided)
        {
            acceleration = stats.wallSlideAcceleration;
            maxFallSpeed = stats.wallSlideMaxSpeed;
        }
        // Free fall
        else
        {
            acceleration = stats.fallingGravity;
            maxFallSpeed = stats.maxFallVelocity;
        }

        if (!isJumpHeld)
            acceleration *= stats.gravityMultiplierWhenRelease;
        
        velocity.y = max(velocity.y + acceleration * dt, maxFallSpeed);
    }
}

void Player::HandleJump()
{
    f64 currTime = AEGetTime(nullptr);

    bool isJumpBufferActive = (currTime - lastJumpPressed) < stats.jumpBuffer;
    bool isCoyoteTimeActive = (currTime - lastGroundedTime) < stats.coyoteTime;
    //bool ifJump = isJumpBufferActive && isCoyoteTimeActive;

    if (!isJumpBufferActive)
        return;

    if (isLeftWallCollided || isRightWallCollided)
    {
        PerformJump();

        bool isInputTowardsWall = (inputDirection.x < 0 && isLeftWallCollided) ||
                                  (inputDirection.x > 0 && isRightWallCollided);

        velocity.x = (isLeftWallCollided ? 1.f : -1.f) * (isInputTowardsWall ? stats.wallJumpHorizontalVelocityTowardsWall : stats.wallJumpHorizontalVelocity);
    }
    else if (isCoyoteTimeActive)
    {
        PerformJump();
    }
}

void Player::PerformJump()
{
    constexpr f64 lowestFloat = std::numeric_limits<f64>::lowest();
    velocity.y = stats.jumpVelocity;
    lastJumpPressed = lowestFloat; // Prevent jump buffer from triggering again
    lastGroundedTime = lowestFloat;
    AEGetTime(&lastJumpTime);
    ifReleaseJumpAfterJumping = false;

    //sprite.SetState(JUMP_START, true);
}

bool Player::IsAnimGroundAttack()
{
    AnimState state = static_cast<AnimState>(sprite.GetState());
    return  state >= ATTACK_1 && state <= ATTACK_END;
}

bool Player::IsAnimAirAttack()
{
    AnimState state = static_cast<AnimState>(sprite.GetState());
    return  state >= AIR_ATTACK_1 && state <= AIR_ATTACK_3;
}

bool Player::IsAttacking()
{
    return IsAnimAirAttack() || IsAnimGroundAttack();
}

void Player::UpdateAttacks()
{
    AttackStats* attack = nullptr;
    AnimState animState = static_cast<AnimState>(sprite.GetState());

    if (IsAnimGroundAttack())
        attack = &stats.groundAttacks[animState - AnimState::ATTACK_1];
    else if (IsAnimAirAttack())
        attack = &stats.groundAttacks[animState - AnimState::AIR_ATTACK_1];
    // Else, not attacking
    else
        return;

    AEVec2 colliderPos;
    AEVec2Add(&colliderPos, &position, &attack->collider.position);
    if (AEInputCheckCurr(AEVK_LCTRL))
        QuickGraphics::DrawRect(colliderPos, attack->collider.size, 0xFFFF0000);
}

void Player::OnAttackAnimEnd(int spriteStateIndex)
{
    AnimState spriteState = static_cast<AnimState>(spriteStateIndex);

    bool inAttackInputBuffer = AEGetTime(nullptr) - lastAttackHeld < stats.attackBuffer;
    bool isLastAttack = spriteState == AnimState::ATTACK_END || spriteState == AnimState::AIR_ATTACK_END;
    
    // If not in attack input buffer OR is last attack in combo, 
    // Reset - Set to idle
    if (!inAttackInputBuffer || isLastAttack)
    {
        sprite.SetState(AnimState::IDLE_W_SWORD);
    }
    else
    {
        sprite.SetState(spriteStateIndex + 1, false, 
            [this](int index) { OnAttackAnimEnd(index); }
        );
    }

    // If transitioning to last attack
    if (spriteState == AnimState::ATTACK_END - 1)
    {
        auto emitter = ParticleSystem::EmitterSettings{
            { position.x + 1.5f, position.x + 1.5f },
            { position.y + 0.5f, position.y + 0.5f },
            { AEDegToRad(-15.f), AEDegToRad(15.f) },
            { 15.f, 20.f },
            { 0.3f, 0.4f }
        };
        particleSystem.SpawnParticleBurst(emitter, 10);
    }
}

void Player::UpdateTrails()
{
    // Update particle system
    float currVelocityAngle;
    if (velocity.y == 0.f)
        currVelocityAngle = (velocity.x > 0) ? AEDegToRad(180.f) : 0.f;
    else if (velocity.x == 0.f)
        currVelocityAngle = (velocity.y > 0) ? AEDegToRad(-90.f) : -90.f;
    else
        currVelocityAngle = atan2f(velocity.x, velocity.y) + AEDegToRad(180.f);

    float angleRange = AEDegToRad(50.f) * 0.5f;
    particleSystem.emitter.angleRange.x = currVelocityAngle - angleRange;
    particleSystem.emitter.angleRange.y = currVelocityAngle + angleRange;

    float speed = AEVec2Length(&velocity);
    particleSystem.emitter.speedRange.x = speed * 0.3f * 0.75f;
    particleSystem.emitter.speedRange.y = speed * 0.3f * 1.5f;
    //std::cout << "Angle: " << AERadToDeg(currVelocityAngle - angleRange) << "\n";
    particleSystem.SetSpawnRate(AEExtras::RemapClamp(speed, { 0.f, stats.maxSpeed }, { -100.f, 50.f }));

    AEVec2Set(&particleSystem.emitter.spawnPosRangeX, position.x + 0.6f, position.x);
    AEVec2Set(&particleSystem.emitter.spawnPosRangeY, position.y - 0.0f, position.y + 1.f);
    particleSystem.Update();
}

void Player::UpdateAnimation()
{
    bool inAttackInputBuffer = AEGetTime(nullptr) - lastAttackHeld < stats.attackBuffer;
    bool isAnimAttack = IsAnimGroundAttack() || IsAnimAirAttack();

    // If player is trying to attack (including input buffer)
    if (inAttackInputBuffer || isAnimAttack)
    {
        if (!isAnimAttack)
        {
            sprite.SetState(AnimState::ATTACK_1, false,
                [this](int index) { OnAttackAnimEnd(index); }
            );
        }
    }
    else
    {
        if ((isLeftWallCollided || isRightWallCollided) && velocity.y != 0.f)
        {
            sprite.SetState(AnimState::WALL_SLIDE);
            
            // If player isn't surrounded by walls
            // Depending on the wall, they are near, make the player face the correct wall
            if (!(isLeftWallCollided && isRightWallCollided))
                facingDirection.x = fabsf(facingDirection.x) * (isLeftWallCollided ? 1.f : -1.f);
        }
        else if (!isGroundCollided)
            sprite.SetState(AnimState::FALLING);
        else if (fabsf(velocity.x) > 0.f)
            sprite.SetState(AnimState::RUN_W_SWORD);
        else
            sprite.SetState(AnimState::IDLE_W_SWORD);

        //std::cout << "vel: " << velocity.y << "\n";
    }

    sprite.Update();
}

void Player::RenderDebugCollider(Box& box)
{
    AEVec2 boxPos = position;
    AEVec2Add(&boxPos, &boxPos, &box.position);
    QuickGraphics::DrawRect(boxPos, box.size);
}

void Player::DrawInspector()
{
    ImGui::Begin("Player");                          // Create a window called "Hello, world!" and append into it.
    
    // ========== Runtime ==========
    if (ImGui::CollapsingHeader("Runtime"))
    {
        ImGui::Text("Physics");
        ImGui::DragFloat2("Position", &position.x, 0.1f);
        ImGui::DragFloat2("Velocity", &velocity.x, 0.1f);

        ImGui::Text("Stats");
        ImGui::SliderInt("Health", &health, 0, stats.maxHealth);
    }

    // ========== Stats ==========
    if (ImGui::CollapsingHeader("Stats"))
    {
        stats.DrawInspector();
    }

    ImGui::End();
}
