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
<<<<<<< Updated upstream
=======
    void Reset(const AEVec2& initialPos);
    void ClearVelocity();

    // === Inspectable ===
    void DrawInspector() override;
    bool CheckIfClicked(const AEVec2& mousePos) override;

    // === IDamageable ===
    const AEVec2& GetHurtboxPos() const override;
    const AEVec2& GetHurtboxSize() const override;
    bool IsDead() const override;
    bool TryTakeDamage(int dmg, const AEVec2& hitOrigin, DAMAGE_TYPE type = DAMAGE_TYPE_ENEMY_ATTACK) override;

    // === Getters ===
    const AEVec2&       GetPosition()   const;
    const PlayerStats&  GetStats()      const;
    float   GetDashCooldownPercentage() const;
    int     GetHealth()         const;
    int     GetMaxHealth()      const;
    float   GetHealthPercentage() const;
    bool    GetIsFacingRight()  const;
    bool    GetIsGrounded()     const;
    AnimState GetAnimState()    const;

    // === Setters ===
    void SetPosition(const AEVec2& pos);

>>>>>>> Stashed changes
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

