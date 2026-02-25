#include "EnemyBoss.h"
#include <cmath>
#include "../../Utils/QuickGraphics.h"
#include "../Camera.h"
#include <AEVec2.h>
#include <Windows.h>
#include <vector>
#include <algorithm>
#include <imgui.h>
#include "../../Utils/AEExtras.h"


static inline u32 ScaleAlpha(u32 argb, float alphaMul)
{
    unsigned a = (argb >> 24) & 0xFF;
    unsigned rgb = argb & 0x00FFFFFF;

    float af = (a / 255.0f) * alphaMul;
    af = max(0.0f, min(1.0f, af));

    unsigned anew = (unsigned)(af * 255.0f + 0.5f);
    return (anew << 24) | rgb;
}

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


static bool g_spellcastUntil5thSpawn = false;
static std::vector<SpecialAttack> g_specialAttacks;

static float GetAnimDurationSec(const Sprite& sprite, int stateIndex)
{
    if (stateIndex < 0 || stateIndex >= sprite.metadata.rows)
        return 0.f;

    const auto& s = sprite.metadata.stateInfoRows[stateIndex];
    return (float)s.frameCount * (float)s.timePerFrame;
}

// NEW (tiny helper): time-per-frame for a state (used to start spawning on last frame)
static float GetAnimTimePerFrame(const Sprite& sprite, int stateIndex)
{
    if (stateIndex < 0 || stateIndex >= sprite.metadata.rows)
        return 0.f;

    const auto& s = sprite.metadata.stateInfoRows[stateIndex];
    return (float)s.timePerFrame;
}

static bool AABB_Overlap2(const AEVec2& aPos, const AEVec2& aSize,
    const AEVec2& bPos, const AEVec2& bSize)
{
    const float dx = std::fabs(aPos.x - bPos.x);
    const float dy = std::fabs(aPos.y - bPos.y);
    return dx <= (aSize.x + bSize.x) * 0.5f
        && dy <= (aSize.y + bSize.y) * 0.5f;
}

bool EnemyBoss::TryTakeDamage(int dmg, const AEVec2& )
{
    if (isDead || dmg <= 0 || invulnTimer > 0.f || hurtTimeLeft > 0.f)
        return false;

    hp -= dmg;

    if (hp <= 0)
    {
        hp = 0;
        isDead = true;
        sprite.SetState(DEATH, false, nullptr);
        deathTimeLeft = GetAnimDurationSec(sprite, DEATH);
        if (deathTimeLeft <= 0.f)
            deathTimeLeft = 0.5f; // fallback
        attack.Reset();
        attack.Reset();
        velocity = AEVec2{ 0.f, 0.f };
        chasing = false;

        teleportActive = false;
        specialBurstActive = false;
        specialSpawnsRemaining = 0;
        specialSpawnTimer = 0.f;
        g_spellcastUntil5thSpawn = false;
        g_specialAttacks.clear();
        // (optional: stop specials/teleport etc)
        return true;
    }

    // start hurt lock
    hurtTimeLeft = GetAnimDurationSec(sprite, HURT);
    if (hurtTimeLeft < minHurtDuration) hurtTimeLeft = minHurtDuration;

    attack.Reset();
    velocity = AEVec2{ 0.f, 0.f };
    chasing = false;

    sprite.SetState(HURT);

    invulnTimer = invulnDuration;
    return true;
}


bool EnemyBoss::TryTakeDamageFromHitbox(const AEVec2& hitPos, const AEVec2& hitSize,
    int dmg)
{
    if (isDead) return false;

    // AABB_Overlap2 assumes pos = center, size = full width/height.
    // matches: DrawRect(position.x, position.y, size.x, size.y, ...)
    if (!AABB_Overlap2(hitPos, hitSize, position, size))
        return false;

    return TryTakeDamage(dmg, {});
}

EnemyBoss::EnemyBoss(float initialPosX, float initialPosY)
    : sprite("Assets/Craftpix/Bringer_of_Death3.png")
    , specialAttackVfx("Assets/Craftpix/Bringer_of_Death3.png")
{
    position = AEVec2{ initialPosX, initialPosY };
    velocity = AEVec2{ 0.f, 0.f };
    specialAttackVfx.SetState(SPELL1);

    size = AEVec2{ 0.8f, 0.8f };
    facingDirection = AEVec2{ 1.f, 0.f };
    chasing = false;

    attack.startRange = 1.8f;
    attack.hitRange = 1.8f;
    attack.cooldown = 0.8f;
    attack.hitTimeNormalized = 1.5f;
    attack.breakRange = 99999.0f;
}

EnemyBoss::~EnemyBoss()
{
}

void EnemyBoss::Update(const AEVec2& playerPos, bool playerFacingRight)
{

	float dt = (float)AEFrameRateControllerGetFrameTime();
    float hpTarget = (maxHP > 0) ? (float)hp / (float)maxHP : 0.f;
    hpTarget = max(0.f, min(1.f, hpTarget));

    // Trigger chip delay ONLY when HP drops this frame
    if (hpTarget < prevHpTarget)
        hpChipDelay = 0.15f;

    prevHpTarget = hpTarget;

    // Front follows fast
    hpBarFront += (hpTarget - hpBarFront) * (1.0f - expf(-18.0f * dt));

    // Delay countdown
    hpChipDelay = max(0.f, hpChipDelay - dt);

    // Chip follows slow AFTER delay
    if (hpChipDelay <= 0.f)
        hpBarChip += (hpBarFront - hpBarChip) * (1.0f - expf(-3.5f * dt));

    // Keep chip >= front (chip is the �old?HP)
    if (hpBarChip < hpBarFront)
        hpBarChip = hpBarFront;

    //clamp just in case
	if (hpBarShown < 0.f) hpBarShown = 0.f;
	if (hpBarShown > 1.5) hpBarShown = 1.5f;


    

    if (isDead)
    {
        // Ensure we are in DEATH state (safe to call; SetState ignores same-state)
        sprite.SetState(DEATH);

        // Same logic as regular Enemy: update until the last frame starts, then stop updating.
        if (deathTimeLeft > 0.f)
        {
            float tpf = GetAnimTimePerFrame(sprite, DEATH);
            if (tpf <= 0.f) tpf = 0.1f;

            if (deathTimeLeft > tpf)
                sprite.Update();

            deathTimeLeft -= dt;
            if (deathTimeLeft < 0.f) deathTimeLeft = 0.f;

			if (deathTimeLeft <= 0.f) hideAfterDeath = true;
        }


        return;
    }

    // Tick invulnerability
    if (invulnTimer > 0.f)
    {
        invulnTimer -= dt;
        if (invulnTimer < 0.f) invulnTimer = 0.f;
    }

    if (hurtTimeLeft > 0.f)
    {
        hurtTimeLeft -= dt;
        if (hurtTimeLeft < 0.f) hurtTimeLeft = 0.f;

        attack.Reset();
        velocity = AEVec2{ 0.f, 0.f };
        chasing = false;

    
        sprite.Update();
        specialAttackVfx.Update();

        if (specialBurstActive)
        {
			sprite.SetState(SPELLCAST);
        }

        // keep specials updating 
        for (auto& s : g_specialAttacks) s.Update(dt);
        g_specialAttacks.erase(
            std::remove_if(g_specialAttacks.begin(), g_specialAttacks.end(),
                [](const SpecialAttack& s) { return !s.alive(); }),
            g_specialAttacks.end()
        );

        return;
    }

    const float desiredStopDist = (attack.startRange > 0.05f) ? (attack.startRange - 0.05f)
        : attack.startRange;

    const float dx = playerPos.x - position.x;
    const float absDx = std::fabs(dx);

    const bool inAggroRange = (absDx <= aggroRange);
	if (inAggroRange) bossEngaged = inAggroRange;

    // --- TELEPORT trigger (only when not in special/attack/etc.) ---
    const bool teleportBlockedBySpecial = g_spellcastUntil5thSpawn;
    // If teleport is active, we fully own the boss this frame.
    if (teleportActive)
    {
        // Freeze everything while teleporting
        attack.Reset();
        chasing = false;
        velocity.x = 0.f;
        velocity.y = 0.f;

        teleportTimer += dt;

        const float teleDur = GetAnimDurationSec(sprite, TELEPORT);
        const float snapTime = (teleDur > 0.f) ? teleDur * teleportMoveNormalized : 0.f;

        // Snap behind player once, mid-animation
        if (!teleportMoved && teleportTimer >= snapTime)
        {
            // If player faces right, behind is left (negative). If faces left, behind is right (positive).
            const float behindDir = playerFacingRight ? -1.f : 1.f;

            position.x = playerPos.x + behindDir * teleportBehindOffset;

            // Boss should face the player after teleport (same direction as player facing)
            facingDirection = AEVec2{ playerFacingRight ? 1.f : -1.f, 0.f };

            teleportMoved = true;
        }

        // End teleport after animation duration, then immediately start ATTACK
        if (teleDur > 0.f && teleportTimer >= teleDur)
        {
            teleportActive = false;
            teleportTimer = 0.f;
            teleportMoved = false;
            teleportCooldownTimer = 0.f;

            // Prime an immediate attack (EnemyAttack auto-starts if in range)
            const float newDx = playerPos.x - position.x;
            const float newAbsDx = std::fabs(newDx);
            const float attackDur = GetAnimDurationSec(sprite, ATTACK);

            attack.Reset();                 // clears cooldownTimer too
            attack.Update(0.f, newAbsDx, attackDur); // dt=0 is fine; it can still start attack
        }

        // Don't let normal animation/movement override TELEPORT this frame.
        sprite.Update();
        specialAttackVfx.Update();

        // Update + cleanup specials (keep your existing block)
        for (auto& specialAttack : g_specialAttacks)
            specialAttack.Update(dt);

        g_specialAttacks.erase(
            std::remove_if(g_specialAttacks.begin(), g_specialAttacks.end(),
                [](const SpecialAttack& specialAttack) { return !specialAttack.alive(); }),
            g_specialAttacks.end()
        );

        return;
    }

    // Not teleporting: build up teleport timer
    if (inAggroRange && !attack.IsAttacking() && !teleportBlockedBySpecial && !specialBurstActive)
    {
        teleportCooldownTimer += dt;

        if (teleportCooldownTimer >= teleportInterval)
        {
            teleportActive = true;
            teleportTimer = 0.f;
            teleportMoved = false;

            // Start teleport animation now
            sprite.SetState(TELEPORT);

            // Freeze immediately
            attack.Reset();
            chasing = false;
            velocity = AEVec2{ 0.f, 0.f };
        }
    }
    else
    {
        // Optional: reset if player leaves aggro / boss is busy
        teleportCooldownTimer = 0.f;
    }


    // Update attack component
    const float attackDur = GetAnimDurationSec(sprite, ATTACK);
    attack.Update(dt, absDx, attackDur);

    // Unlock special sequence after boss performs its first normal attack
    if (attack.JustStarted() && !specialUnlocked)
    {
        specialUnlocked = true;
        SpecialElapsed = 0.0f;
        specialBurstActive = false;
        specialSpawnsRemaining = 0;
        specialSpawnTimer = 0.0f;
    }

    // --- Special sequence ---
    static constexpr float kSpecialCooldown = 5.0f;
    static constexpr float kSpecialSpawnGap = 1.0f;   // 0.5s between spawns
    static constexpr int   kSpecialSpawnCount = 5;

    if (specialUnlocked)
    {
        if (specialBurstActive)
        {
            // Face player during special
            if (dx != 0.f)
                facingDirection = AEVec2{ (dx > 0.f) ? 1.f : -1.f, 0.f };

            // Timer handles: initial cast delay + spawn gaps
            specialSpawnTimer -= dt;

            while (specialSpawnTimer <= 0.0f && specialSpawnsRemaining > 0)
            {
                const float dir = (facingDirection.x >= 0.f) ? 1.f : -1.f;

                SpecialAttack specialAttack;
                specialAttack.pos = AEVec2{ position.x + dir * 0.6f, position.y + 0.35f };
                specialAttack.vel = AEVec2{ dir * 7.0f, 0.f };
                specialAttack.radius = 0.14f;
                specialAttack.life = 1.6f;
                specialAttack.color = 0xFFAA66FF;

                g_specialAttacks.push_back(specialAttack);

                --specialSpawnsRemaining;
                if (specialSpawnsRemaining <= 0) g_spellcastUntil5thSpawn = false; // ? stop spellcast as soon as 5th is spawned

                specialSpawnTimer += kSpecialSpawnGap;
            }

            if (specialSpawnsRemaining <= 0)
            {
                specialBurstActive = false;
                SpecialElapsed = 0.0f; // cooldown begins after burst completes
            }
        }
        else
        {
            SpecialElapsed += dt;

            if (SpecialElapsed >= kSpecialCooldown)
            {
                // Start special
                specialBurstActive = true;
                specialSpawnsRemaining = kSpecialSpawnCount;
                SpecialElapsed = 0.0f;
                g_spellcastUntil5thSpawn = true;


                // Start SPELLCAST now
                sprite.SetState(SPELLCAST);

                // IMPORTANT CHANGE:
                // Start spawning on the *last frame start* of SPELLCAST so there's no visible "dead gap"
                const float castDur = GetAnimDurationSec(sprite, SPELLCAST);
                const float tpf = GetAnimTimePerFrame(sprite, SPELLCAST);
                specialSpawnTimer = (castDur > 0.f) ? max(0.0f, castDur - tpf) : 0.0f;
            }
        }
    }

    // Special phase should keep SPELLCAST + stop movement as long as:
    // - burst is active OR
    // - special attacks are still alive on screen
    const bool specialPhaseActive = g_spellcastUntil5thSpawn; // ? only cast until 5th spawn is created


    // If attacking OR special phase, stop movement
    if (attack.IsAttacking() || specialPhaseActive)
    {
        // While special phase is active, prevent normal attacks/movement
        if (specialPhaseActive)
            attack.Reset();

        // Keep facing player during special phase
        if (specialPhaseActive && dx != 0.f)
            facingDirection = AEVec2{ (dx > 0.f) ? 1.f : -1.f, 0.f };

        velocity.x = 0.f;
        velocity.y = 0.f;
        chasing = false;
    }
    else
    {
        if (inAggroRange)
        {
            chasing = (absDx > desiredStopDist);

            // always face player
            if (dx != 0.f)
                facingDirection = AEVec2{ (dx > 0.f) ? 1.f : -1.f, 0.f };

            if (chasing)
            {
                const float dirX = (dx > 0.f) ? 1.f : -1.f;
                velocity.x = dirX * moveSpeed;
            }
            else
            {
                velocity.x = 0.f;
            }

            velocity.y = 0.f;

            AEVec2 displacement;
            AEVec2Scale(&displacement, &velocity, dt);
            AEVec2 nextPos = position;
            AEVec2Add(&nextPos, &position, &displacement);

            if (chasing)
            {
                const float dirX = (dx > 0.f) ? 1.f : -1.f;
                const float targetX = playerPos.x - dirX * desiredStopDist;

                if (dirX > 0.f && nextPos.x > targetX) { nextPos.x = targetX; velocity.x = 0.f; }
                if (dirX < 0.f && nextPos.x < targetX) { nextPos.x = targetX; velocity.x = 0.f; }
            }

            position = nextPos;
        }
        else
        {
            chasing = false;
        }
    }

    // Animation selection:
    // As long as special phase is active, keep SPELLCAST looping (reset/replay is OK)
    if (!specialPhaseActive && !teleportActive)
        UpdateAnimation();
    // else: do NOT override SPELLCAST (it loops via sprite.Update())

    sprite.Update();
    specialAttackVfx.Update();

    // Update + cleanup specials
    for (auto& specialAttack : g_specialAttacks)
        specialAttack.Update(dt);

    g_specialAttacks.erase(
        std::remove_if(g_specialAttacks.begin(), g_specialAttacks.end(),
            [](const SpecialAttack& specialAttack) { return !specialAttack.alive(); }),
        g_specialAttacks.end()
    );
}

int EnemyBoss::ConsumeSpecialHits(const AEVec2& playerPos, const AEVec2& playerSize)
{
    if (isDead) return 0;

    int hits = 0;

    for (auto& s : g_specialAttacks)
    {
        if (!s.alive()) continue;

        // Use the same debug rect size you draw (so collision matches visuals)
        const AEVec2 spellSize{ s.debugW, s.debugH };

        if (AABB_Overlap2(s.pos, spellSize, playerPos, playerSize))
        {
            ++hits;

            // Consume the projectile so it only hits once
            s.life = 0.f;
        }
    }

    return hits;
}



void EnemyBoss::UpdateAnimation()
{
    if (teleportActive)
    {
        // Only do this if you are NOT already setting TELEPORT once at teleport start.
        sprite.SetState(TELEPORT);
        return;
    }

    if (attack.IsAttacking())
    {
        sprite.SetState(ATTACK);
        return;
    }

    if (chasing) sprite.SetState(RUN);
    else         sprite.SetState(IDLE);
}


void EnemyBoss::Render()
{

    if (hideAfterDeath) return;

    AEMtx33 m;

    const bool faceRight =
        (velocity.x != 0.f) ? (velocity.x > 0.f) : (facingDirection.x > 0.f);

    const float bossScale = 3.5f;

    AEMtx33Scale(&m, faceRight ? bossScale : -bossScale, bossScale);

    float px = sprite.metadata.pivot.x;
    float py = sprite.metadata.pivot.y;

    if (!faceRight)
        px = 1.0f - px;

    AEMtx33TransApply(
        &m, &m,
        position.x - (0.5f - px),
        position.y - (0.75f - py)
    );

    AEMtx33ScaleApply(&m, &m, Camera::scale, Camera::scale);
    AEGfxSetTransform(m.m);

    sprite.Render();

    // Reset transform for world-space debug / effects
    AEMtx33 world;
    AEMtx33Scale(&world, Camera::scale, Camera::scale);
    AEGfxSetTransform(world.m);

    for (const auto& specialAttack : g_specialAttacks)
    {
        specialAttack.Render(specialAttackVfx, 10.0f, 3.0f);

        AEGfxSetTransform(world.m);

        if (debugDraw)
        {
            QuickGraphics::DrawRect(
                specialAttack.pos.x, specialAttack.pos.y,
                specialAttack.debugW, specialAttack.debugH,
                0xFF00FF00,
                AE_GFX_MDM_LINES_STRIP
            );

            QuickGraphics::DrawRect(
                specialAttack.pos.x, specialAttack.pos.y,
                0.05f, 0.05f,
                0xFFFF0000,
                AE_GFX_MDM_LINES_STRIP
            );
        }
    }

    AEGfxSetTransform(world.m);

    if (debugDraw)
    {
        const u32 color = chasing ? 0xFFFF4040 : 0xFFB0B0B0;
        QuickGraphics::DrawRect(position.x, position.y, size.x, size.y, color, AE_GFX_MDM_LINES_STRIP);
    }
    RenderHealthbar();

    // make sure we�re not stuck in additive mode from VFX
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
}

void EnemyBoss::DrawInspector()
{
    ImGui::Begin("EnemyBoss");

    if (ImGui::CollapsingHeader("Runtime"))
    {
        ImGui::DragFloat2("Position", &position.x, 0.1f);
        ImGui::DragFloat2("Velocity", &velocity.x, 0.1f);

        ImGui::Checkbox("Dead", &isDead);
        ImGui::Checkbox("Attacking", &isAttacking);
        ImGui::Checkbox("Grounded", &isGrounded);
        ImGui::Checkbox("Chasing", &chasing);
    }

    if (ImGui::CollapsingHeader("HP"))
    {
        ImGui::SliderInt("MaxHP", &hp, 0, maxHP);
        ImGui::Text("MaxHP: %d", maxHP);
    }

	if (ImGui::CollapsingHeader("Attack"))
    {
        ImGui::SliderInt("AttackDamage", &attackDamage, 0, 20);
        ImGui::DragFloat("StartRange", &attack.startRange, 0.05f, 0.f, 10.f);
        ImGui::DragFloat("HitRange", &attack.hitRange, 0.05f, 0.f, 10.f);
        ImGui::DragFloat("Cooldown", &attack.cooldown, 0.05f, 0.f, 10.f);
        ImGui::DragFloat("HitTimeNorm", &attack.hitTimeNormalized, 0.05f, 0.1f, 3.f);
        ImGui::DragFloat("BreakRange", &attack.breakRange, 0.05f, 0.f, 20.f);
    }

    if (ImGui::CollapsingHeader("Tuning"))
    {
        ImGui::DragFloat("AggroRange", &aggroRange, 0.05f, 0.f, 100.f);
        ImGui::DragFloat("MoveSpeed", &moveSpeed, 0.05f, 0.f, 20.f);

        ImGui::SeparatorText("Teleport");
        ImGui::DragFloat("TeleportInterval", &teleportInterval, 0.05f, 0.1f, 10.f);
        ImGui::DragFloat("BehindOffset", &teleportBehindOffset, 0.05f, 0.f, 5.f);

        ImGui::SeparatorText("Debug");
        ImGui::Checkbox("DebugDraw", &debugDraw);
        ImGui::Checkbox("ShowHealthbar", &showHealthbar);
    }

    ImGui::End();
}

bool EnemyBoss::CheckIfClicked(const AEVec2& mousePos)
{
    return fabsf(position.x - mousePos.x) < size.x &&
        fabsf(position.y - mousePos.y) < size.y;
}

void EnemyBoss::RenderHealthbar() const
{
	if (!showHealthbar) return;
    if (hideAfterDeath) return;
    if (!bossEngaged) return;
      
    AEMtx33 ui;
    AEMtx33Scale(&ui, 1.f, 1.f);
    AEGfxSetTransform(ui.m);

    // --- HUD anchor in screen pixels ---
    const float screenW = (float)AEGfxGetWindowWidth();

    const float topMarginPx = 30.f;   // distance from top of screen
    const float barPxW = 520.f;  // pixel width
    const float barPxH = 18.f;   // pixel height

    AEVec2 barCenterWorld;
    AEExtras::ScreenToWorldPosition({ screenW * 0.5f, topMarginPx + barPxH * 0.5f }, barCenterWorld);

    AEVec2 leftWorld, rightWorld, topWorld, bottomWorld;
    AEExtras::ScreenToWorldPosition({ screenW * 0.5f - barPxW * 0.5f, topMarginPx + barPxH * 0.5f }, leftWorld);
    AEExtras::ScreenToWorldPosition({ screenW * 0.5f + barPxW * 0.5f, topMarginPx + barPxH * 0.5f }, rightWorld);
    AEExtras::ScreenToWorldPosition({ screenW * 0.5f, topMarginPx }, topWorld);
    AEExtras::ScreenToWorldPosition({ screenW * 0.5f, topMarginPx + barPxH }, bottomWorld);

    const float barW = fabsf(rightWorld.x - leftWorld.x);
    const float barH = fabsf(bottomWorld.y - topWorld.y);

    const float barCenterX = barCenterWorld.x;
    const float barY = barCenterWorld.y;

    const float barLeftX = barCenterX - barW * 0.5f;
    //const float fillW = barW * hpBarShown;
    //const float fillCenterX = barLeftX + fillW * 0.5f;

    float chipW = barW * hpBarChip;
    float frontW = barW * hpBarFront;

    float chipCenterX = barLeftX + chipW * 0.5f;
    float frontCenterX = barLeftX + frontW * 0.5f;
    AEGfxSetBlendMode(AE_GFX_BM_BLEND); // safety

    // Background
    QuickGraphics::DrawRect(barCenterX, barY, barW, barH, 0xFFFFFFFF, AE_GFX_MDM_TRIANGLES);

    // Border 
   QuickGraphics::DrawRect(barCenterX, barY, barW, barH, 0xFFFFFFFF, AE_GFX_MDM_LINES_STRIP);

    

    if (frontW > 0.001f)
    {
        // Chip (delayed)
        QuickGraphics::DrawRect(chipCenterX, barY, chipW, barH, 0xFF7A0000, AE_GFX_MDM_TRIANGLES);

        QuickGraphics::DrawRect(frontCenterX, barY, frontW, barH, 0xFFFF0000, AE_GFX_MDM_TRIANGLES);
    }
   

    // Text (optional)
   // (Top-left label + HP numbers)
    std::string label = "Bringer of Death";
    QuickGraphics::PrintText(label.c_str(), -0.95f, 0.88f, 0.35f, 1, 1, 1, 1);
    /*
    std::string hpStr = std::to_string(hp) + " / " + std::to_string(maxHP);
    QuickGraphics::PrintText(hpStr.c_str(), 0.55f, 0.88f, 0.35f, 1, 1, 1, 1);
    */
}
