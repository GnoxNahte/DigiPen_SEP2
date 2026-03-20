#pragma once
#include "../../Utils/Sprite.h" 
#include "EnemyAttack.h"
#include <AEVec2.h>
#include "IDamageable.h"
#include "../../Editor/EditorUtils.h"
#include "../../Utils/ParticleSystem.h"
#include "../../Utils/Box.h"
#include "../Camera.h"

class MapGrid; // forward declaration to avoid circular dependency


struct SpecialAttack
{
    AEVec2 pos{ 0.f, 0.f };
    AEVec2 vel{ 0.f, 0.f };
    float  radius{ 0.16f };
    float  life{ 1.5f };
    u32    color{ 0xFF66CCFF };

    float debugW{ 0.5f };
    float debugH{ 1.20f };

    bool alive() const { return life > 0.f; }

    void Update(float dt)
    {
        AEVec2 disp;
        AEVec2Scale(&disp, &vel, dt);
        AEVec2Add(&pos, &pos, &disp);
        life -= dt;
    }

    void Render(Sprite& vfx, float sx, float sy) const
    {
        AEMtx33 m;
        const bool faceRight = (vel.x >= 0.0f);

        AEMtx33Scale(&m, sx, sy);

        float px = vfx.metadata.pivot.x;
        float py = vfx.metadata.pivot.y;

        if (!faceRight)
            px = 1.0f - px;

        AEMtx33TransApply(
            &m, &m,
            pos.x - (0.5f - px),
            pos.y - (0.5f - py)
        );

        AEMtx33ScaleApply(&m, &m, Camera::scale, Camera::scale);
        AEGfxSetTransform(m.m);

        AEGfxSetBlendMode(AE_GFX_BM_ADD);
        vfx.Render();
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    }
};

class EnemyBoss : public IDamageable, Inspectable
{
public:
    EnemyBoss(float initialPosX, float initialPosY);
    EnemyBoss();
    ~EnemyBoss();

    void Update(const AEVec2& playerPos, bool playerFacingRight, MapGrid& map);
    void Reset(const AEVec2& spawnPos);

    void SetSpawnPosition(const AEVec2& spawnPos);

    void Render();
    
    AEVec2 position{};


    bool isDead = false;
    bool isAttacking = false;
    bool isGrounded = true;
    bool chasing = false;
    int attackDamage = 10;

    //raise to start chasing player
    float aggroRange = 10.0f;
    float aggroYRange = 1.0f;
    float attackYRange = 0.6f;

    bool bossEngaged{ false }; // becomes true once player enters aggro range
    bool phase2 = false;
    float phase2HpThreshold = 0.60f;

    //particle systemmmmm
    ParticleSystem particleSystem{ 30, {} };
    void SpawnImpactBurst();
    void SpawnSpecialTrail(const SpecialAttack& s);
    void SpawnSpecialImpactBurst(const AEVec2& hitPos);
    void SpawnSpecialMuzzleBurst(const AEVec2& spawnPos, float dir);
    void SpawnSpellChargeVfx(float dt);

    bool IsDead() const override { return isDead; }
    //for gamescene to use to apply damage later
    bool PollAttackHit() 
    { 
        if (isDead) return false;

        if (attack.PollHit())   // PollHit() is the "consume once" moment
        {
            SpawnImpactBurst();
            return true;
        }
        return false;
    }

    // returns number of special projectiles that hit the player this frame
    int ConsumeSpecialHits(const AEVec2& playerPos, const AEVec2& playerSize);
 

    // Single hurtbox for now (same as your debug rect in Render()).
    const AEVec2& GetHurtboxPos() const { return position; }
    const AEVec2& GetHurtboxSize() const { return size; }
    int GetHP() const { return hp; }
    int GetMaxHP() const { return maxHP; }

	//IMGUI inspector
    void DrawInspector() override;
    bool CheckIfClicked(const AEVec2& mousePos) override;

    int   GetAttackDamage() const { return attackDamage; }
    bool IsInvulnerable() const { return invulnTimer > 0.f; }
    bool TryTakeDamage(int dmg, const AEVec2& hitOrigin, DAMAGE_TYPE type = DAMAGE_TYPE_NORMAL) override;
    // Convenience: checks overlap vs boss hurtbox first, then applies damage.
    bool TryTakeDamageFromHitbox(const AEVec2& hitPos, const AEVec2& hitSize,
        int dmg);

    const Box& GetMeleeHitbox() const { return meleeHitbox; }
    void UpdateMeleeHitbox(const AEVec2& playerPos); // call each frame (or during attack

    void RebuildTeleportBounds();

    bool IsTeleportXValid(float targetX,
        const AEVec2& playerPos,
        MapGrid& map,
        bool allowOnPlayer = false) const;

    float FindTeleportTarget(const AEVec2& playerPos,
        bool playerFacingRight,
        MapGrid& map) const;

private:

    enum AnimState
    {
        IDLE = 0,
        RUN = 1,
        ATTACK = 2,
        HURT = 3,
        TELEPORT = 4,
        SPELLCAST = 5,
        SPELL1 = 6,
        DEATH = 7
    };
    void UpdateAnimation();

    AEVec2 velocity{ 0.f, 0.f };
    Sprite sprite;
    Sprite specialAttackVfx;   // extra sprite used only for special attack VFX

    AEVec2 facingDirection{ 1.f, 0.f };


    float width{ 0.8f };
    float height{ 0.8f };

    float moveSpeed{ 2.2f };
    float stopDistance{ 0.2f };

       // --- Teleport control ---
    float teleportCooldownTimer{ 0.0f };   // counts up until teleport triggers
    float teleportInterval{ 2.0f };        // teleport every 2s (tune)
    bool  teleportActive{ false };         // currently playing TELEPORT anim
    float teleportTimer{ 0.0f };           // time since TELEPORT started
    bool  teleportMoved{ false };          // have we snapped behind player yet?

    float teleportBehindOffset{ 0.9f };    // how far behind player (tune)
    float teleportMoveNormalized{ 0.5f };  // when to snap (0..1 of teleport anim)

    // spawn-anchor teleport limits
    float spawnAnchorX{ 0.0f };
    float teleportHalfRange{ 4.0f };      // tune to cage
    float teleportMinX{ 0.0f };
    float teleportMaxX{ 0.0f };
    float teleportWallPadding{ 0.10f };
    float teleportMinPlayerGap{ 0.30f };



    EnemyAttack attack;

    // --- Special attack sequence control ---
    float SpecialElapsed{ 0.0f };           // cooldown timer (seconds)
    bool specialUnlocked{ false };          // becomes true after first normal attack
    bool specialBurstActive{ false };       // currently spawning the 5 specials
    int  specialSpawnsRemaining{ 0 };       // how many left in this burst
    float specialSpawnTimer{ 0.0f };        // time until next spawn in burst


    // Debug / collider size (use AEVec2 like Player)
    AEVec2 size{ 0.8f, 0.8f };
    bool debugDraw{ false };

    // --- Health / damage ---
    int   maxHP{ 1000 };
    int   hp{ 1000 };

    // Hurt lock (like regular Enemy): keeps HURT animation visible + blocks re-hits.
    float hurtTimeLeft{ 0.0f };
    float minHurtDuration{ 0.30f };

    float invulnTimer{ 0.0f };
    float invulnDuration{ 0.20f };

    float deathTimeLeft{ 0.0f };

	bool hideAfterDeath{ false };

    // --- Boss UI (screen-space healthbar) ---
    float hpBarShown{ 1.0f };          // 0..1 smoothed display
    float hpBarDrainPerSec{ 0.60f };   // smaller = slower drain (tune)
    bool  showHealthbar{ true };       // optional toggle
    void RenderHealthbar() const;
   
    float hpBarFront = 1.0f;   // fast bar (actual HP)
    float hpBarChip = 1.0f;   // delayed bar (chip trail)
    float hpChipDelay = 0.0f; // delay before chip starts shrinking
    float prevHpTarget = 1.0f;
    //UI transitions ----
    s8 bossFont = -1;
    bool hudIntroStarted = false;
    bool bossHudVisible = false;
    float hudIntroTimer = 0.0f;
    float nameFadeDuration = 1.5f;   // first: name fades in
    float barStartDelay = 0.35f;    // wait before bar starts
    float barFillDuration = 1.5f;    // then bar fills

    Box meleeHitbox{ AEVec2{0,0}, AEVec2{1.4f, 0.9f} }; // size tuned later

  
};

