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

Player::Player(MapGrid* map, EnemyManager* enemyManager) :
    stats("Assets/config/player-stats.json"), 
    sprite("Assets/Art/rvros/Adventurer.png"),
    facingDirection{},
    inputDirection{},
    transform{},
    velocity{},
    particleSystem{ 50, {} },
    map(map),
    enemyManager(enemyManager)
{
    Reset(AEVec2{ 2, 4 });

    particleSystem.Init();
    particleSystem.emitter.lifetimeRange.x = 0.1f;
    particleSystem.emitter.lifetimeRange.y = 0.3f;

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

    UpdateInput();
    UpdateTriggerColliders();

    // Update velocity
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

    // Local scale. For flipping sprite's facing direction
    bool ifFaceRight = (velocity.x != 0.f && !IsAttacking()) ? (velocity.x > 0) : (facingDirection.x > 0);
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
    
    // maybe remove, dash time is quite short player might not notice
    if (IsInvincible())
        AEGfxSetTransparency(0.5f);

    sprite.Render();

    if (Editor::GetShowColliders())
    {
        RenderDebugCollider(stats.groundChecker);
        RenderDebugCollider(stats.ceilingChecker);
        RenderDebugCollider(stats.leftWallChecker);
        RenderDebugCollider(stats.rightWallChecker);
        QuickGraphics::DrawRect(position, stats.playerSize, 0xFFFF0000, AE_GFX_MDM_LINES_STRIP);
    }

    AEGfxSetTransparency(1.f);


    // TODO - DELETE. just for testing dynamic map collision
    AEVec2 mousePos;
    AEExtras::GetCursorWorldPosition(mousePos);
    mousePos.y += 0.5f * stats.playerSize.y;
    AEVec2 collidedPos = position;
    AEVec2 vel = mousePos - position;
    map->HandleBoxCollision(collidedPos, vel, mousePos, stats.playerSize);
    //QuickGraphics::DrawRect(mousePos, stats.playerSize, ifCollide ? 0xFFFF0000 : 0xFF00FF00);
    QuickGraphics::DrawRect(collidedPos, stats.playerSize, 0x4400FF00);
    QuickGraphics::DrawRect(mousePos, stats.playerSize, 0x44FF0000);
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

	lastAttackHeld = std::numeric_limits<f64>::lowest(); 
	dashStartTime = std::numeric_limits<f64>::lowest();

    health = stats.maxHealth;
    hasAppliedRecoil = false;
    lastDamagedTime = std::numeric_limits<f64>::lowest();

    buff_MoveSpeedMulti = 1.f;
    buff_DmgReduction = 1.f;
    buff_critChance = 1.f;
    buff_critDmgMulti = 1.f;
    buff_DmgMultiLowHP = 1.f;

    attackedEnemies.clear();
    sprite.SetState(AnimState::IDLE_W_SWORD);
    particleSystem.ReleaseAll();
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

bool Player::IsFacingRight() const
{
    // match your Render() logic
    if (velocity.x != 0.f) return velocity.x > 0.f;
    return facingDirection.x > 0.f;
}

Player::AnimState Player::GetAnimState() const
{
    return static_cast<AnimState>(sprite.GetState());
}


void Player::UpdateInput()
{
    float currTime = static_cast<float>(Time::GetInstance().GetScaledElapsedTime());

    // Consider shift all keybinds to another file. Then maybe can allow custom keybinding 
    inputDirection.x = (f32)((AEInputCheckCurr(AEVK_RIGHT) || AEInputCheckCurr(AEVK_D))
                     - (AEInputCheckCurr(AEVK_LEFT) || AEInputCheckCurr(AEVK_A)));
    inputDirection.y = (f32)((AEInputCheckCurr(AEVK_UP) || AEInputCheckCurr(AEVK_W))
                     - (AEInputCheckCurr(AEVK_DOWN) || AEInputCheckCurr(AEVK_S)));

    isJumpHeld = AEInputCheckCurr(AEVK_SPACE) || AEInputCheckCurr(AEVK_C);
    if (AEInputCheckTriggered(AEVK_SPACE) || AEInputCheckTriggered(AEVK_C))
        lastJumpPressed = currTime;

    if ((inputDirection.x != 0 || inputDirection.y != 0) && (!IsAttacking() || AEInputCheckTriggered(AEVK_Z)))
        facingDirection = inputDirection;

    if (AEInputCheckCurr(AEVK_X))
        lastAttackHeld = currTime;

    if (AEInputCheckCurr(AEVK_Z) && currTime - dashStartTime > stats.dashCooldown + stats.dashTime)
        dashStartTime = currTime;
}

void Player::UpdateTriggerColliders()
{
    AEVec2 tmpPosition;

    bool wasGroundCollided = isGroundCollided;
    AEVec2Add(&tmpPosition, &position, &stats.groundChecker.position);
    isGroundCollided = map->CheckBoxCollision(tmpPosition, stats.groundChecker.size);

    // If in air -> grounded
    if (!wasGroundCollided && isGroundCollided)
        HandleLanding();

    AEVec2Add(&tmpPosition, &position, &stats.ceilingChecker.position);
    isCeilingCollided = map->CheckBoxCollision(tmpPosition, stats.ceilingChecker.size);

    AEVec2Add(&tmpPosition, &position, &stats.leftWallChecker.position);
    isLeftWallCollided = map->CheckBoxCollision(tmpPosition, stats.leftWallChecker.size);

    AEVec2Add(&tmpPosition, &position, &stats.rightWallChecker.position);
    isRightWallCollided = map->CheckBoxCollision(tmpPosition, stats.rightWallChecker.size);
}

void Player::HorizontalMovement()
{
    float dt = static_cast<float>(Time::GetInstance().GetScaledDeltaTime());
    f64 currTime = Time::GetInstance().GetScaledElapsedTime();

    float dashPercentage = static_cast<float>((currTime - dashStartTime) / stats.dashTime);

    if (dashPercentage < 1.f)
    {
        float speed = stats.dashSpeed * buff_MoveSpeedMulti;
        // If moving in the same direction
        if (inputDirection.x * velocity.x >= 0)
            speed *= 1.5f;
        if (facingDirection.x < 0.f)
            speed = -speed;
        velocity.x = speed;

        /*float value = 1 - dashPercentage * dashPercentage;
        velocity.x = AEExtras::RemapClamp(value, { 0.f, 1.f }, { stats.dashSpeed, stats.maxSpeed });*/
    }
    // Slow player down when not pressing any buttons
    else if (inputDirection.x == 0)
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
            if (IsAttacking())
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
    float angleRange = 5.f;
    if (state == AnimState::AIR_ATTACK_SMASH)
    {
        ParticleSystem::EmitterSettings emitter{
            .spawnPosRangeX { position.x, position.x },
            .spawnPosRangeY { position.y - 0.2f, position.y + 0.3f },
            .angleRange     { AEDegToRad(0.f), AEDegToRad(angleRange) },
            .speedRange     { 2.f, 30.f },
            .lifetimeRange  { 0.1f, 0.3f },
            .tint           { 0.56f, 0.49f, 0.77f, 1.f }
        };
        particleSystem.SpawnParticleBurst(emitter, 25);

        emitter.angleRange.x = AEDegToRad(180.f);
        emitter.angleRange.y = AEDegToRad(180.f - angleRange);
        particleSystem.SpawnParticleBurst(emitter, 25);
    }

    // @todo: (Ethan) - Play sound
}

void Player::HandleGravity()
{
    float dt = static_cast<float>(Time::GetInstance().GetScaledDeltaTime());

    // If moving up
    if (velocity.y > 0.f)
    {
        if (isCeilingCollided)
            velocity.y = 0;
        else
        {
            float velocityChange = stats.gravity * dt;
            if (!isJumpHeld && (static_cast<float>(Time::GetInstance().GetScaledElapsedTime()) - lastJumpTime) > stats.minJumpTime)
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
            maxFallSpeed = stats.downAirAttackFallSpeed;
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
    f64 currTime = static_cast<float>(Time::GetInstance().GetScaledElapsedTime());

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
    lastJumpTime = static_cast<float>(Time::GetInstance().GetScaledElapsedTime());
    ifReleaseJumpAfterJumping = false;

    //sprite.SetState(JUMP_START, true);
}

void Player::UpdateCollisions(const AEVec2& nextPosition)
{
    // === Map collisions ===
    map->HandleBoxCollision(position, velocity, nextPosition, stats.playerSize);
    
    // === Enemy collisions ===
    if (IsDashing())
        return;

    IDamageable* damageable = IfCollideEnemy({ position, stats.playerSize });
    if (damageable)
        TryTakeDamage(5, damageable->GetHurtboxPos());
}

bool Player::IsDashing()
{
    f64 currTime = Time::GetInstance().GetScaledElapsedTime();
    return currTime - dashStartTime < stats.dashTime;
}

bool Player::IsAnimGroundAttack()
{
    AnimState state = GetAnimState();
    return  state >= ATTACK_1 && state <= ATTACK_END;
}

bool Player::IsAnimAirAttack()
{
    AnimState state = GetAnimState();
    return  state >= AIR_ATTACK_1 && state <= AIR_ATTACK_END;
}

bool Player::IsAttacking()
{
    return IsAnimAirAttack() || IsAnimGroundAttack();
}

bool Player::IsInvincible()
{
    return IsDashing() || Time::GetInstance().GetScaledElapsedTime() - lastDamagedTime < stats.invincibleTime;
}

void Player::SetAttack(AnimState toState)
{
    sprite.SetState(toState, false,
        [this](int index) { OnAttackAnimEnd(index); }
    );
}

void Player::AttackDamageable(IDamageable& damageable, const AttackStats& attack, bool isGroundAttack)
{
    int damage = attack.damage;
    // When low health, increase damage
    // todo - Change to precalculate? make percentage into another variable too
    if (health < 0.2f * stats.maxHealth)
        damage = static_cast<int>(damage * buff_DmgMultiLowHP);

    // Crit
    if (AERandFloat() < buff_critChance)
        damage = static_cast<int>(damage * buff_critDmgMulti);

    damageable.TryTakeDamage(damage, position);
    attackedEnemies.push_back(&damageable);

    if (!hasAppliedRecoil)
    {
        if (isGroundAttack)
            velocity.x += facingDirection.x > 0 ? -attack.recoilSpeed : attack.recoilSpeed;
        else
            velocity.y += attack.recoilSpeed;

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
        position.x + attack->collider.position.x * (facingDirection.x > 0 ? 1.f : -1.f),
        position.y + attack->collider.position.y
    };

    // Check if attack hit enemy
    if (enemyManager)
    {
        enemyManager->ForEachDamageable([&](IDamageable& obj) {
            // If hit enemy && current enemy isn't in attackedEnemies
            bool ifAttack = PhysicsUtils::AABB(colliderPos, attack->collider.size, obj.GetHurtboxPos(), obj.GetHurtboxSize()) &&
                            std::find(attackedEnemies.cbegin(), attackedEnemies.cend(), &obj) == attackedEnemies.cend();

            if (ifAttack)
                AttackDamageable(obj, *attack, isGroundAttack);
        });
    }

    if (Editor::GetShowColliders())
        QuickGraphics::DrawRect(colliderPos, attack->collider.size, 0xFF8888FF);
}

void Player::OnAttackAnimEnd(int spriteStateIndex)
{
    attackedEnemies.clear();
    hasAppliedRecoil = false;

    AnimState spriteState = static_cast<AnimState>(spriteStateIndex);

    bool inAttackInputBuffer = static_cast<float>(Time::GetInstance().GetScaledElapsedTime()) - lastAttackHeld < stats.attackBuffer;
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
        SetAttack(static_cast<AnimState>(spriteStateIndex + 1));

        // Shouldn't handle input here but not sure how else to do..
        // If switch direction when chaining attacks
        if (((AEInputCheckCurr(AEVK_LEFT) || AEInputCheckCurr(AEVK_A)) && facingDirection.x > 0) ||
            ((AEInputCheckCurr(AEVK_RIGHT) || AEInputCheckCurr(AEVK_D)) && facingDirection.x < 0))
            facingDirection.x *= -1;
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
    if (IsAttacking() || IsDead())
    {
        sprite.Update();
        return;
    }

    // If player is trying to attack (including input buffer)
    if (static_cast<float>(Time::GetInstance().GetScaledElapsedTime()) - lastAttackHeld < stats.attackBuffer)
    {
        if (!isGroundCollided && (AEInputCheckCurr(AEVK_DOWN) || AEInputCheckCurr(AEVK_S)))
            SetAttack(AIR_ATTACK_SMASH);
        else
            SetAttack(ATTACK_1);
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

    switch (card.type)
    {
    case CARD_TYPE::HERMES_FAVOR:   buff_MoveSpeedMulti     *= PercentToScale(card.effectValue1); break;
    case CARD_TYPE::IRON_DEFENCE:   buff_DmgReduction       *= 1.f - (card.effectValue1 / 100.f); break;
    case CARD_TYPE::REVITALIZE:     health = stats.maxHealth; break;
    case CARD_TYPE::SHARPEN:        
        buff_critDmgMulti   *= PercentToScale(card.effectValue1);
        buff_critChance     *= PercentToScale(card.effectValue2); 
        break;
    case CARD_TYPE::BERSERKER:      buff_DmgMultiLowHP      *= PercentToScale(card.effectValue1); break;
    //case CARD_TYPE::FEATHERWEIGHT: break;
    
    // Not handling
    case CARD_TYPE::SWITCH_IT_UP: 
        break;

    default:
        std::cout << "Player.OnBuffSelectedEvent - Unknown card type\n";
        break;
    }
}

float Player::PercentToScale(int percentage)
{
    return 1.f + percentage / 100.f;
}

const AEVec2& Player::GetHurtboxPos()  const { return position; }
const AEVec2& Player::GetHurtboxSize() const { return stats.playerSize; }
bool Player::IsDead() const { return GetAnimState() == AnimState::DEATH || GetAnimState() == AnimState::DEATH_LOOP; }

bool Player::TryTakeDamage(int dmg, const AEVec2& hitOrigin)
{
    if (IsInvincible())
        return health > 0;

    lastDamagedTime = Time::GetInstance().GetScaledElapsedTime();
    UI::GetDamageTextSpawner().SpawnDamageText(dmg, DAMAGE_TYPE_ENEMY_ATTACK, position);

    if (health < dmg)
    {
        health = 0;
        EventSystem::Trigger<PlayerDeathEvent>({ *this });
        sprite.SetState(AnimState::DEATH, false, [&](int) {
            sprite.SetState(AnimState::DEATH_LOOP); 
        });
        return false;
    }

    health -= dmg;
    
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
        ImGui::DragFloat2("Facing direction", &facingDirection.x, 0.1f);

        ImGui::SeparatorText("Stats");
        ImGui::SliderInt("Health", &health, 0, stats.maxHealth);
        ImGui::TextDisabled("Is attacking: %s", IsAttacking() ? "Y" : "N");
        ImGui::TextDisabled("Sprite index: %s", std::to_string(sprite.GetState()).c_str());
        
        ImGui::SeparatorText("Buffs");
        ImGui::DragFloat("Move Speed", &buff_MoveSpeedMulti, 0.1f);
        ImGui::DragFloat("Damage Reduction", &buff_DmgReduction, 0.1f);
        ImGui::DragFloat("Crit Chance", &buff_critChance, 0.1f);
        ImGui::DragFloat("Crit Dmg", &buff_critDmgMulti, 0.1f);
        ImGui::DragFloat("Damage Low HP", &buff_DmgMultiLowHP, 0.1f);
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
