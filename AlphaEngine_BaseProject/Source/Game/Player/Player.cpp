#include <iostream>
#include <limits>
#include <imgui.h>

#include "Player.h"
#include "../Camera.h"
#include "../../Utils/QuickGraphics.h"
#include "../../Utils/AEExtras.h"
#include "../../Utils/Event/EventSystem.h"
#include "../../Editor/Editor.h"
#include "../../Game/Time.h"
#include "../UI.h"
#include "../../Utils/PhysicsUtils.h"
#include "../AudioManager.h"

namespace
{
    float PercentToScale(int percentage)
    {
        return 1.f + percentage / 100.f;
    }

    float PercentToScaleInvert(int percentage)
    {
        return 1.f - percentage / 100.f;
    }
}

Player::Player(MapGrid* map, EnemyManager* enemyManager) :
    stats("Assets/config/player-stats.json"), 
    sprite("Assets/Art/rvros/Adventurer.png"),
    particleSystem{ 50, {} },
    afterimage(*this, {2.f, 2.f * 0.74f}),
    map(map),
    enemyManager(enemyManager)
{
    Reset(AEVec2{ 10, 10 });
    SetKeybinds();

    particleSystem.Init();
    particleSystem.emitter.lifetimeRange.x = 0.1f;
    particleSystem.emitter.lifetimeRange.y = 0.3f;
    particleSystem.emitter.tint.a = 0.5f;

    buffEventId = EventSystem::Subscribe<BuffSelectedEvent>([this](const BuffSelectedEvent& ev) {
        OnBuffSelected(ev);
    });
}

Player::~Player()
{
    EventSystem::Unsubscribe<BuffSelectedEvent>(buffEventId);
}

void Player::Update()
{
    if (IsDead())
    {
        UpdateAnimation();
        return;
    }

    if (Time::GetInstance().GetTimeScale() == 0.f)
        return;

    UpdateInput();
    UpdateTriggerColliders();

    // Update velocity
    if (IsDashing())
    {
        AEVec2 speed = stats.dashSpeed * buff_MoveSpeedMulti;
        // If moving in the same direction
        if (inputDirection.x * velocity.x >= 0)
            speed *= 1.5f;
        if (inputDirection.y * velocity.y >= 0)
            speed *= 1.5f;

        // If no input from player, dash in the player facing direction
        if (AEExtras::IsZero(inputDirection))
            velocity = AEVec2{ isFacingRight ? speed.x : -speed.x, 0.f };
        // Else dash using inputDirection
        else
            velocity = AEExtras::Normalise(inputDirection) * speed;
    }

    HorizontalMovement();
    VerticalMovement();

    UpdateAttacks();

    // Update position based on velocity
    AEVec2 displacement, nextPosition;
    AEVec2Scale(&displacement, &velocity, static_cast<float>(Time::GetInstance().GetScaledDeltaTime()));
    AEVec2Add(&nextPosition, &position, &displacement);
    UpdateCollisions(nextPosition);

    UpdateAnimation();
    UpdateTrails();
}

void Player::Render()
{
    particleSystem.Render();
    afterimage.Render();

    // Local scale. For flipping sprite's facing direction
    // Multiply height by 0.74f because sprite aspect ratio isn't a square
    constexpr float scale = 2.f;
    AEMtx33Scale(&transform, isFacingRight ? scale : -scale, scale * 0.74f);
    AEMtx33TransApply(
        &transform,
        &transform,
        position.x - (0.5f - sprite.metadata.pivot.x),
        position.y + (0.5f - sprite.metadata.pivot.y)
    );
    // Camera scale. Scales translation too.
    AEMtx33ScaleApply(&transform, &transform, Camera::scale, Camera::scale);
    AEGfxSetTransform(transform.m);
    
    // maybe remove, dash time is quite short player might not notice
    if (IsInvincible())
        AEGfxSetTransparency(0.5f);

    sprite.Render();

    AEGfxSetTransparency(1.f);

    if (Editor::GetShowColliders())
    {
        //RenderDebugCollider(stats.groundChecker);
        //RenderDebugCollider(stats.ceilingChecker);
        //RenderDebugCollider(stats.leftWallChecker);
        //RenderDebugCollider(stats.rightWallChecker);
        QuickGraphics::DrawRect(position, stats.playerSize, 0xFFFF0000, AE_GFX_MDM_LINES_STRIP);
    }

    //if (AEInputCheckTriggered(AEVK_K))
    //    std::cout << "A";

    //// Test dynamic collision
    //AEVec2 mousePos;
    //AEExtras::GetCursorWorldPosition(mousePos);
    //mousePos.y += 0.5f * stats.playerSize.y;
    //AEVec2 collidedPos = position;
    //AEVec2 vel = mousePos - position;
    //map->HandleBoxCollision(collidedPos, vel, mousePos, stats.playerSize, true);
    //QuickGraphics::DrawRect(collidedPos, stats.playerSize, 0x4400FF00);
    //QuickGraphics::DrawRect(mousePos, stats.playerSize, 0x44FF0000);
    //std::cout << position << "     " << collidedPos << vel << "\n";

    //if (AEInputCheckTriggered(AEVK_RBUTTON))
    //    position = collidedPos;
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

    isFacingRight = true;
    lastJumpTime = -1.f;
    lastGroundedTime = -1.f;

    isGroundCollided = false;
    isCeilingCollided = false;
    isLeftWallCollided = false;
    isRightWallCollided = false;

	lastAttackHeld = std::numeric_limits<f64>::lowest(); 
	dashStartTime = std::numeric_limits<f64>::lowest();

    health = maxHealth = stats.maxHealth;
    hasAppliedRecoil = false;
    lastDamagedTime = std::numeric_limits<f64>::lowest();
    lastAttackEndTime = std::numeric_limits<f64>::lowest();
    lastAttackCombo = AnimState::ATTACK_1;
    slamStartHeight = std::numeric_limits<float>::lowest();

    buff_MoveSpeedMulti = 1.f;
    buff_DmgReduction = 1.f;
    buff_TrapDmgReduction = 1.f;
    buff_critChance = 0.f;
    buff_critDmgMulti = 1.5f;
    buff_CritChanceLowHP = 0.f;
    buff_DmgMulti = 1.f;
    buff_DashCooldownMulti = 1.f;

    attackedEnemies.clear();
    sprite.SetState(AnimState::IDLE_W_SWORD);
    particleSystem.ReleaseAll();

    afterimage.ResetLastSpawn();
}

const AEVec2& Player::GetPosition() const
{
    return position;
}

int Player::GetHealth() const
{
    return health;
}

int Player::GetMaxHealth() const
{
    return maxHealth;
}

float Player::GetHealthPercentage() const
{
    return static_cast<float>(health) / maxHealth;
}

const PlayerStats& Player::GetStats() const
{
    return stats;
}

const Sprite& Player::GetSprite() const
{
    return sprite;
}

bool Player::GetIsFacingRight() const
{
    return isFacingRight;
}

bool Player::GetIsGrounded() const
{
    return isGroundCollided;
}

bool Player::IsDashing() const
{
    f64 currTime = Time::GetInstance().GetScaledElapsedTime();
    return !IsAnimAirAttack() && (currTime - dashStartTime < stats.dashTime);
}

float Player::GetDashCooldownPercentage() const
{
    float timeSinceDash = static_cast<float>(Time::GetInstance().GetScaledElapsedTime() - dashStartTime);
    float percentage = timeSinceDash / (stats.dashCooldown * buff_DashCooldownMulti + stats.dashTime);
    return AEClamp(percentage, 0.f, 1.f);
}

Player::AnimState Player::GetAnimState() const
{
    return static_cast<AnimState>(sprite.GetState());
}

void Player::SetPosition(const AEVec2& pos)
{
    position = pos;

    AEVec2Zero(&velocity);
    UpdateTriggerColliders();

    afterimage.ResetLastSpawn();
}

void Player::UpdateInput()
{
    float currTime = static_cast<float>(Time::GetInstance().GetScaledElapsedTime());

    // Consider shift all keybinds to another file. Then maybe can allow custom keybinding 
    inputDirection.x = static_cast<f32>(AEInputCheckCurr(keybinds.right)) - AEInputCheckCurr(keybinds.left);
    inputDirection.y = static_cast<f32>(AEInputCheckCurr(keybinds.up)) - AEInputCheckCurr(keybinds.down);

    isJumpHeld = AEInputCheckCurr(keybinds.jump);
    if (AEInputCheckTriggered(keybinds.jump))
        lastJumpPressed = currTime;

    if (inputDirection.x != 0 && (!IsAttacking() || AEInputCheckTriggered(keybinds.dash)))
        isFacingRight = inputDirection.x > 0;

    if (AEInputCheckCurr(keybinds.attack))
        lastAttackHeld = currTime;

    if (AEInputCheckCurr(keybinds.dash) && currTime - dashStartTime > stats.dashCooldown * buff_DashCooldownMulti + stats.dashTime) {
        AudioManager::PlaySFX(*AudioManager::playerDash, AudioManager::GetSFXVolume());
        dashStartTime = currTime;
        afterimage.ResetLastSpawn();
    }
}

void Player::UpdateTriggerColliders()
{
    bool wasGroundCollided = isGroundCollided;
    isGroundCollided = map->CheckBoxCollision(position + stats.groundChecker.position, stats.groundChecker.size);

    // If in air -> grounded
    if (!wasGroundCollided && isGroundCollided)
        HandleLanding();

    isCeilingCollided   = map->CheckBoxCollision(position + stats.ceilingChecker.position,  stats.ceilingChecker.size);
    isLeftWallCollided  = map->CheckBoxCollision(position + stats.leftWallChecker.position, stats.leftWallChecker.size);
    isRightWallCollided = map->CheckBoxCollision(position + stats.rightWallChecker.position,stats.rightWallChecker.size);
}

void Player::SetKeybinds()
{
    keybinds.left = AEVK_A;
    keybinds.right = AEVK_D;
    keybinds.up = AEVK_W;
    keybinds.down = AEVK_S;

    keybinds.jump = AEVK_SPACE;
    keybinds.dash = AEVK_LSHIFT;
    keybinds.attack = AEVK_LBUTTON;
}

void Player::HorizontalMovement()
{
    if (IsDashing())
        return;

    float dt = static_cast<float>(Time::GetInstance().GetScaledDeltaTime());

    // Slow player down when not pressing any buttons
    if (inputDirection.x == 0)
    {
        float velocityChange = stats.stopAcceleration * dt * buff_MoveSpeedMulti;
        // -velocityChange because stats.stopAcceleration < 0, so negate that
        if (fabsf(velocity.x) > -velocityChange)
            velocity.x += velocityChange * (velocity.x > 0.f ? 1.f : -1.f);
        // If velocity.x < -velocityChange, set to 0 to make sure it won't overshoot
        else
            velocity.x = 0.f;

        float maxSpeed = stats.maxSpeed * buff_MoveSpeedMulti;
        velocity.x = AEClamp(velocity.x, -maxSpeed, maxSpeed);
    }
    else
    {
        // If moving in the same direction
        if (inputDirection.x * velocity.x >= 0)
        {
            velocity.x += stats.moveAcceleration * inputDirection.x * dt * buff_MoveSpeedMulti;

            float maxSpeed = (isGroundCollided ? stats.maxSpeed : stats.airStrafeMaxSpeed) * buff_MoveSpeedMulti;
            if (IsAttacking() && isGroundCollided)
                maxSpeed *= stats.attackMaxSpeedMultiplier;
            velocity.x = AEClamp(velocity.x, -maxSpeed, maxSpeed);
        }
        // Moving in opposite direction
        else
        {
            float acceleration = (isGroundCollided ? stats.turnAcceleration : stats.inAirTurnAcceleration) * buff_MoveSpeedMulti;
            velocity.x -= acceleration * inputDirection.x * dt;
        }
    }
}

void Player::VerticalMovement()
{
    HandleGravity();
    HandleJump();
}

void Player::HandleLanding()
{
    int state = sprite.GetState();
    
    // Default values, will set later depending on the landing
    int particleSpawnCount = 0;
    float particleAngleRange = 0.f;
    AEVec2 particleSpeedRange;
    
    // Alot of code below will use this format: A + B * scale
    // A - base value
    // B - variable value based on scale
    if (state == AnimState::AIR_ATTACK_SMASH)
    {
        float smashStrength = GetSlamAttackScale();
        particleSpawnCount = 10 + static_cast<int>(20 * smashStrength);
        particleAngleRange = 10.f + 15.f * smashStrength;
        AEVec2Set(&particleSpeedRange, 2.f, 10.f + 20.f * smashStrength);

        Camera::StartShake(0.25f, 0.25f * smashStrength);

        // Lower pitch the stronger the smash strength
        float pitch = 1.f - smashStrength * 0.5f;
        float volume = (0.75f + 0.5f * smashStrength) * AudioManager::GetSFXVolume();
        AudioManager::PlaySFX(*AudioManager::playerAirAttackImpact, volume, pitch);
    }
    else
    {
        float landStrength = AEClamp(velocity.y / stats.maxFallVelocity, 0.f, 1.f);
        particleSpawnCount = static_cast<int>(2 + 3 * landStrength);
        particleAngleRange = 5.f + 10.f * landStrength;
        AEVec2Set(&particleSpeedRange, 2.f, 2.f + landStrength * 5.f);
        AudioManager::PlaySFX(*AudioManager::playerLand, AudioManager::GetSFXVolume(), AEExtras::RandomRange({0.8f, 1.4f}));
    }

    ParticleSystem::EmitterSettings emitter{
        .spawnPosRangeX { position.x, position.x },
        .spawnPosRangeY { position.y - 0.1f, position.y },
        .angleRange     { AEDegToRad(0.f), AEDegToRad(particleAngleRange) },
        .speedRange     { particleSpeedRange },
        .lifetimeRange  { 0.1f, 0.3f },
        .tint           { 0.56f, 0.49f, 0.77f, 1.f }
    };

    particleSystem.SpawnParticleBurst(emitter, particleSpawnCount);

    emitter.angleRange.x = AEDegToRad(180.f);
    emitter.angleRange.y = AEDegToRad(180.f - particleAngleRange);
    particleSystem.SpawnParticleBurst(emitter, particleSpawnCount);
}

void Player::HandleGravity()
{
    if (IsDashing())
        return;

    float dt = static_cast<float>(Time::GetInstance().GetScaledDeltaTime());
    
    // If moving up
    if (velocity.y > 0.f)
    {
        if (isCeilingCollided)
            velocity.y = 0;
        else
        {
            float velocityChange = stats.gravity * dt;
            if (!isJumpHeld && (Time::GetInstance().GetScaledElapsedTime() - lastJumpTime) > stats.minJumpTime)
                velocityChange *= stats.gravityMultiplierWhenRelease;

            velocity.y += velocityChange;
        }
    }
    // If on ground
    else if (isGroundCollided)
    {
        lastGroundedTime = (f32)Time::GetInstance().GetScaledElapsedTime();
        velocity.y = 0.f;
    }
    // Else, falling
    else
    {
        float acceleration, maxFallSpeed;
        if (IsAnimAirAttack())
        {
            acceleration = -200.f;
            maxFallSpeed = stats.slamAttackFallSpeed;
        }
        // Wall slide
        else if (isLeftWallCollided || isRightWallCollided)
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
    if (IsAnimAirAttack())
        return;

    f64 currTime = static_cast<float>(Time::GetInstance().GetScaledElapsedTime());

    bool isJumpBufferActive = (currTime - lastJumpPressed) < stats.jumpBuffer;
    bool isCoyoteTimeActive = (currTime - lastGroundedTime) < stats.coyoteTime;
    //bool ifJump = isJumpBufferActive && isCoyoteTimeActive;

    if (!isJumpBufferActive)
        return;

    if ((isLeftWallCollided || isRightWallCollided) && inputDirection.x != 0)
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
    lastJumpTime = static_cast<float>(Time::GetInstance().GetScaledElapsedTime());
    ifReleaseJumpAfterJumping = false;

    AudioManager::PlaySFX(*AudioManager::playerJump, AudioManager::GetSFXVolume(), AEExtras::RandomRange({0.8f, 1.4f}));
}

void Player::UpdateCollisions(const AEVec2& nextPosition)
{
    // === Map collisions ===
    map->HandleBoxCollision(position, velocity, nextPosition, stats.playerSize, true);
    
    // === Enemy collisions ===
    if (IsDashing() || IsAnimAirAttack())
        return;

    IDamageable* damageable = IfCollideEnemy({ position, stats.playerSize });
    if (damageable)
        TryTakeDamage(5, damageable->GetHurtboxPos());
}

bool Player::IsAnimGroundAttack() const
{
    AnimState state = GetAnimState();
    return  state >= ATTACK_1 && state <= ATTACK_END;
}

bool Player::IsAnimAirAttack() const
{
    AnimState state = GetAnimState();
    return  state >= AIR_ATTACK_1 && state <= AIR_ATTACK_END;
}

bool Player::IsAttacking() const
{
    return IsAnimAirAttack() || IsAnimGroundAttack();
}

bool Player::IsInvincible() const
{
    return IsDashing() || Time::GetInstance().GetScaledElapsedTime() - lastDamagedTime < stats.invincibleTime;
}

void Player::SetAttack(AnimState toState)
{
    if (toState == AIR_ATTACK_SMASH)
        slamStartHeight = position.y;
    
    //AudioManager::PlayNextAttackSFX();

    sprite.SetState(toState, false,
        [this](int index) { OnAttackAnimEnd(index); }
    );

    float pitch = AEExtras::RandomRange({0.8f, 1.4f});
    float volume = AudioManager::GetSFXVolume();
    switch (toState)
    {
    case ATTACK_1: AudioManager::PlaySFX(*AudioManager::playerAttack1, volume, pitch); break;
    case ATTACK_2: AudioManager::PlaySFX(*AudioManager::playerAttack2, volume, pitch); break;
    case ATTACK_3: AudioManager::PlaySFX(*AudioManager::playerAttack3, volume, pitch); break;
    case AIR_ATTACK_SMASH: AudioManager::PlaySFX(*AudioManager::playerAirAttack, volume, pitch); break;
    }
}

void Player::AttackDamageable(IDamageable& damageable, const AttackStats& attack, bool isGroundAttack)
{
    int damage = static_cast<int>(attack.damage * buff_DmgMulti);

    if (!isGroundAttack)
        damage = static_cast<int>(damage * GetSlamAttackScale());

    // Increased crit chacne if low health
    float critChance = buff_critChance + (health <= stats.berserkerTrigger * maxHealth) * buff_CritChanceLowHP;
    bool isCrit =  AERandFloat() < critChance;

    // Crit
    if (isCrit)
    {
        damage = static_cast<int>(damage * buff_critDmgMulti * stats.baseCritDmgMultiplier);
        damageable.TryTakeDamage(damage, position, DAMAGE_TYPE_CRIT);
    }
    else
    {
        damageable.TryTakeDamage(damage, position, DAMAGE_TYPE_NORMAL);
    }

    attackedEnemies.push_back(&damageable);

    if (!hasAppliedRecoil)
    {
        if (isGroundAttack)
            velocity.x += isFacingRight ? -attack.recoilSpeed : attack.recoilSpeed;

        hasAppliedRecoil = true;
    }
}

void Player::UpdateAttacks()
{
    const AttackStats* attack = nullptr;
    AnimState animState = GetAnimState();
    bool isGroundAttack = false;

    if (IsAnimGroundAttack())
    {
        attack = &stats.groundAttacks[animState - AnimState::ATTACK_1];
        isGroundAttack = true;
    }
    else if (IsAnimAirAttack())
    {
        attack = &stats.airAttacks[animState - AnimState::AIR_ATTACK_1];
        isGroundAttack = false;
    }
    // Else, not attacking
    else
        return;

    AEVec2 colliderPos{ 
        position.x + attack->collider.position.x * (isFacingRight ? 1.f : -1.f),
        position.y + attack->collider.position.y
    };

    // Check if attack hit enemy
    if (enemyManager)
    {
        enemyManager->ForEachDamageable([&](IDamageable& obj) {
            // If hit enemy && current enemy isn't in attackedEnemies
            bool ifAttack = PhysicsUtils::AABB(colliderPos, attack->collider.size, obj.GetHurtboxPos(), obj.GetHurtboxSize()) && // If hit enemy
                            std::find(attackedEnemies.cbegin(), attackedEnemies.cend(), &obj) == attackedEnemies.cend() && // If haven't attacked that enemy
                            !map->CheckRaycast(position, obj.GetHurtboxPos());  // If there's no wall in between them
            
            if (ifAttack)
                AttackDamageable(obj, *attack, isGroundAttack);
        });
    }

    if (Editor::GetShowColliders())
        QuickGraphics::DrawRect(colliderPos, attack->collider.size, 0xFF8888FF);
}

void Player::OnAttackAnimEnd(int spriteStateIndex)
{
    AnimState spriteState = static_cast<AnimState>(spriteStateIndex);

    attackedEnemies.clear();
    hasAppliedRecoil = false;
    lastAttackEndTime = Time::GetInstance().GetScaledElapsedTime();
    lastAttackCombo = spriteState;

    bool inAttackInputBuffer = Time::GetInstance().GetScaledElapsedTime() - lastAttackHeld < stats.attackBuffer;
    bool isLastAttack = spriteState == AnimState::ATTACK_END || spriteState == AnimState::AIR_ATTACK_END;
    
    // If not in attack input buffer OR is last attack in combo, 
    // Reset - Set to idle
    if (!inAttackInputBuffer || isLastAttack)
    {
        sprite.SetState(AnimState::IDLE_W_SWORD);
        return;
    }

    if (IsAnimGroundAttack())
    {
        if (inputDirection.y < 0)
            SetAttack(AnimState::AIR_ATTACK_SMASH);
        else
            SetAttack(static_cast<AnimState>(spriteStateIndex + 1));

        // Shouldn't handle input here but not sure how else to do..
        // If switch direction when chaining attacks
        if ((AEInputCheckCurr(keybinds.left)  && isFacingRight) ||
            (AEInputCheckCurr(keybinds.right) && !isFacingRight))
            isFacingRight = !isFacingRight;
    }
}

IDamageable* Player::IfCollideEnemy(const Box& collider)
{
    IDamageable* collidedEnemy = nullptr;

    // Will override collidedEnemy but not solving for now
    enemyManager->ForEachDamageable([&](IDamageable& obj) {
        if (!obj.IsDead() && PhysicsUtils::AABB(collider.position, collider.size, obj.GetHurtboxPos(), obj.GetHurtboxSize()))
            collidedEnemy = &obj;
    });

    return collidedEnemy;
}

float Player::GetSlamAttackScale()
{
    return AEClamp((slamStartHeight - position.y) / stats.slamAttackMaxHeight, 0.1f, 1.f);
}

// Update particle system
void Player::UpdateTrails()
{
    // Update Angle 
    float baseAngle = (velocity.x > 0) ? AEDegToRad(180.f) : 0.f;
    float angleRange = AEDegToRad(50.f) * Sign(velocity.x);
    particleSystem.emitter.angleRange.x = baseAngle;
    particleSystem.emitter.angleRange.y = baseAngle - angleRange;

    // Update Speed
    float speed = AEVec2Length(&velocity);
    particleSystem.emitter.speedRange.x = speed * 0.3f * 0.75f;
    particleSystem.emitter.speedRange.y = speed * 0.3f * 1.5f;

    // Update spawn rate
    if (isGroundCollided)
        particleSystem.SetSpawnRate(AEExtras::RemapClamp(speed, { 0.f, stats.maxSpeed }, { -100.f, 50.f }));
    else 
        particleSystem.SetSpawnRate(0.f);

    // Update spawn pos
    AEVec2Set(&particleSystem.emitter.spawnPosRangeX, position.x + 0.6f, position.x);
    AEVec2Set(&particleSystem.emitter.spawnPosRangeY, position.y, position.y);

    particleSystem.Update();

    afterimage.Update();
}

void Player::UpdateAnimation()
{
    //std::cout << GetAnimState();

    if (IsDead() || IsAttacking())
    {
        sprite.Update();
        return;
    }

    f64 time = Time::GetInstance().GetScaledElapsedTime();
    // If player is trying to attack (including input buffer)
    if (time - lastAttackHeld < stats.attackBuffer)
    {
        if (!isGroundCollided && AEInputCheckCurr(keybinds.down))
            SetAttack(AIR_ATTACK_SMASH);
        else if (time - lastAttackEndTime < stats.attackComboBuffer && lastAttackCombo != AnimState::ATTACK_END)
            SetAttack(static_cast<AnimState>(lastAttackCombo + 1));
        else if (!IsAnimGroundAttack())
            SetAttack(ATTACK_1);
    }
    else
    {
        // If is wall climbing/sliding
        if ((isLeftWallCollided || isRightWallCollided) && velocity.y != 0.f)
        {
            sprite.SetState(AnimState::WALL_SLIDE);
            
            // If player isn't surrounded by walls (but is near at least 1 wall)
            // Depending on the wall, they are near, make the player face the correct wall
            if (!(isLeftWallCollided && isRightWallCollided))
                isFacingRight = isLeftWallCollided;
        }
        else if (!isGroundCollided)
            sprite.SetState(AnimState::FALLING);
        else if (fabsf(velocity.x) > 0.f)
            sprite.SetState(AnimState::RUN_W_SWORD);
        else
            sprite.SetState(AnimState::IDLE_W_SWORD);
    }

    sprite.Update();
}

void Player::RenderDebugCollider(Box& box)
{
    AEVec2 boxPos = position;
    AEVec2Add(&boxPos, &boxPos, &box.position);
    QuickGraphics::DrawRect(boxPos, box.size, 0xFF00FF00, AE_GFX_MDM_LINES_STRIP);
}

void Player::OnBuffSelected(const BuffSelectedEvent& ev)
{
    const BuffCard& card = ev.card;

    std::cout << "Player applying buff - " << BuffCardManager::CardTypeToString(card.type) << "(" << card.effectValue1 << "," << card.effectValue2 << ")" << "\n";
    int healAmt{};
    switch (card.type)
    {
    case CARD_TYPE::HERMES_FAVOR:   buff_MoveSpeedMulti     *= PercentToScale(card.effectValue1); break;
    case CARD_TYPE::IRON_DEFENCE:   buff_DmgReduction       *= PercentToScaleInvert(card.effectValue1); break;
    case CARD_TYPE::REVITALIZE: {
        healAmt = min(static_cast<int>(card.effectValue1 / 100.f * maxHealth), maxHealth - health);
        EventSystem::Subscribe<OverlayFadeCompleteEvent>([this, healAmt](const OverlayFadeCompleteEvent& ev) {
            (void)ev;
            UI::GetDamageTextSpawner().SpawnDamageText(healAmt, DAMAGE_TYPE_HEAL, position, { 0.f, 1.f }); });
        health += healAmt; 
        break;
    }
    case CARD_TYPE::SHARPEN:        
        buff_DmgMulti *= PercentToScale(card.effectValue1);
        buff_critChance     += card.effectValue2 / 100.f; 
        break;
    case CARD_TYPE::BERSERKER:      buff_CritChanceLowHP    += card.effectValue1 / 100.f; break;
    case CARD_TYPE::FLEETING_STEP:  buff_DashCooldownMulti  *= PercentToScaleInvert(card.effectValue1); break;
    case CARD_TYPE::SUREFOOTED:     buff_TrapDmgReduction   *= PercentToScaleInvert(card.effectValue1); break;
    case CARD_TYPE::DEEP_VITALITY: {
        int newMaxHealth = static_cast<int>(maxHealth * PercentToScale(card.effectValue1));
        float percentage = static_cast<float>(health) / maxHealth;
        healAmt = static_cast<int>(percentage * newMaxHealth - health);
        EventSystem::Subscribe<OverlayFadeCompleteEvent>([this, healAmt](const OverlayFadeCompleteEvent& ev) {
            (void)ev;
            UI::GetDamageTextSpawner().SpawnDamageText(healAmt, DAMAGE_TYPE_HEAL, position, { 0.f, 1.f }); });
        health = static_cast<int>(percentage * newMaxHealth);
        maxHealth = newMaxHealth;
        break;
    }
    case CARD_TYPE::HAND_OF_FATE:   buff_critChance     += card.effectValue1 / 100.f; break;
    case CARD_TYPE::SUNDERING_BLOW: buff_critDmgMulti   *= PercentToScale(card.effectValue1); break;
    
    // Not handling
    case CARD_TYPE::SWITCH_IT_UP: 
        break;

    default:
        std::cout << "Player.OnBuffSelectedEvent - Unknown card type\n";
        break;
    }
}

const AEVec2& Player::GetHurtboxPos()  const { return position; }
const AEVec2& Player::GetHurtboxSize() const { return stats.playerSize; }
bool Player::IsDead() const { return health <= 0; }

bool Player::TryTakeDamage(int dmg, const AEVec2& hitOrigin, DAMAGE_TYPE type)
{
    if (IsInvincible() || health <= 0)
        return health > 0;

    dmg = static_cast<int>(dmg * buff_DmgReduction);

    lastDamagedTime = Time::GetInstance().GetScaledElapsedTime();
    UI::GetDamageTextSpawner().SpawnDamageText(dmg, type, position, position - hitOrigin);

    if (health <= stats.berserkerTrigger * maxHealth)
        dmg = static_cast<int>(dmg * stats.berserkerHealthReductionAmt);
    if (health <= dmg)
    {
        health = 0;
        AudioManager::PlaySFX(*AudioManager::playerDeath, AudioManager::GetSFXVolume());
        EventSystem::Trigger<PlayerDeathEvent>({ *this });
        sprite.SetState(AnimState::DEATH, false, [&](int) {
            sprite.SetState(AnimState::DEATH_LOOP); 
        });
        return false;
    }

    health -= dmg;
    float pitch = AEExtras::RandomRange({ 0.9f, 1.2f });
    AudioManager::PlaySFX(*AudioManager::playerHurt, AudioManager::GetSFXVolume(), pitch);
    
    float knockbackStrength = max(dmg / stats.maxKnockbackDmg, 1.f) * stats.knockbackAmt;

    // Calculate knockback direction based on hit origin
    AEVec2 hitDirection = position - hitOrigin;
    AEVec2Normalize(&hitDirection, &hitDirection);
    hitDirection.y = max(hitDirection.y, 0.4f);
    velocity = hitDirection * knockbackStrength;


    sprite.SetState(AnimState::HURT);

    return true;
}

void Player::DrawInspector()
{
    ImGui::Begin("Player", &isInspectorOpen); 
    
    // === Runtime ===
    if (ImGui::CollapsingHeader("Runtime"))
    {
        ImGui::SeparatorText("Physics");
        ImGui::DragFloat2("Position", &position.x, 0.1f);
        ImGui::DragFloat2("Velocity", &velocity.x, 0.1f);
        ImGui::Checkbox("Is facing right", &isFacingRight);

        ImGui::SeparatorText("Stats");
        ImGui::SliderInt("Health", &health, 0, maxHealth);
        ImGui::TextDisabled("Max Health: %d", maxHealth);
        ImGui::TextDisabled("Is attacking: %s", IsAttacking() ? "Y" : "N");
        ImGui::TextDisabled("Sprite index: %s", std::to_string(sprite.GetState()).c_str());
        
        ImGui::SeparatorText("Buffs");
        ImGui::DragFloat("Move Speed", &buff_MoveSpeedMulti, 0.1f);
        ImGui::DragFloat("Damage Reduction", &buff_DmgReduction, 0.1f);
        ImGui::DragFloat("Trap Damage Reduction", &buff_TrapDmgReduction, 0.1f);
        ImGui::DragFloat("Crit Chance", &buff_critChance, 0.1f);
        ImGui::DragFloat("Crit Dmg", &buff_critDmgMulti, 0.1f);
        ImGui::DragFloat("Crit Chance Low HP", &buff_CritChanceLowHP, 0.1f);
        ImGui::DragFloat("Damage", &buff_DmgMulti, 0.1f);
        ImGui::DragFloat("Dash Cooldown Multi", &buff_DashCooldownMulti, 0.1f);
    }

    // === Stats ===
    if (ImGui::CollapsingHeader("Stats"))
    {
        stats.DrawInspector();
    }

    ImGui::End();
}

bool Player::CheckIfClicked(const AEVec2& mousePos)
{
    return  fabsf(position.x - mousePos.x) < stats.playerSize.x &&
            fabsf(position.y - mousePos.y) < stats.playerSize.y;
}
