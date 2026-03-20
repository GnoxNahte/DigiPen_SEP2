#include "EnemyBoss.h"
#include <cmath>
#include "../../Utils/QuickGraphics.h"
#include <AEVec2.h>
#include <Windows.h>
#include <vector>
#include <algorithm>
#include <imgui.h>
#include "../../Utils/AEExtras.h"
#include "../Environment/MapGrid.h"


static inline u32 ScaleAlpha(u32 argb, float alphaMul)
{
    unsigned a = (argb >> 24) & 0xFF;
    unsigned rgb = argb & 0x00FFFFFF;

    float af = (a / 255.0f) * alphaMul;
    af = max(0.0f, min(1.0f, af));

    unsigned anew = (unsigned)(af * 255.0f + 0.5f);
    return (anew << 24) | rgb;
}




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



// EnemyBoss.cpp
void EnemyBoss::UpdateMeleeHitbox(const AEVec2& playerPos)
{
    const AEVec2 bPos = GetHurtboxPos();
    const AEVec2 bSize = GetHurtboxSize();

    // decide side by player (more reliable than facing when boss teleports)
    const float dirX = (playerPos.x >= bPos.x) ? 1.f : -1.f;

    meleeHitbox.size = AEVec2{ 1.4f, 0.9f }; // tweak
    meleeHitbox.position.x = bPos.x + dirX * (bSize.x * 0.5f + meleeHitbox.size.x * 0.5f - 0.10f);
    meleeHitbox.position.y = bPos.y + 0.10f;
}

bool EnemyBoss::TryTakeDamage(int dmg, const AEVec2&, DAMAGE_TYPE )
{
    if (isDead || dmg <= 0 || teleportActive || invulnTimer > 0.f || hurtTimeLeft > 0.f)
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

static inline float Clamp01(float t)
{
    if (t < 0.f) return 0.f;
    if (t > 1.f) return 1.f;
    return t;
}

static inline float Lerp(float a, float b, float t)
{
    return a + (b - a) * Clamp01(t);
}


EnemyBoss::EnemyBoss()
    : sprite("Assets/Craftpix/Bringer_of_Death3.png")
    , specialAttackVfx("Assets/Craftpix/Bringer_of_Death3.png")
    , bossFont(AEGfxCreateFont("Assets/m04.ttf", 36))
{
    //position = AEVec2{ initialPosX, initialPosY };
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

    //particle systemmmm setup
    particleSystem.Init();
    particleSystem.SetSpawnRate(0.f); // IMPORTANT: no continuous spawning by default

    // Optional: default "dust/blood-ish" lifetime for bursts
    particleSystem.emitter.lifetimeRange.x = 0.10f;
    particleSystem.emitter.lifetimeRange.y = 0.25f;



}

struct TeleportArea
{
    float minX;   // leftmost allowed boss center
    float maxX;   // rightmost allowed boss center
    float y;      // boss teleport row, if fixed
};


EnemyBoss::EnemyBoss(float initialPosX, float initialPosY) : EnemyBoss()
{

    position = AEVec2{ initialPosX, initialPosY };
    RebuildTeleportBounds();
  
  

}

EnemyBoss::~EnemyBoss()
{
    AEGfxDestroyFont(bossFont);
}

void EnemyBoss::SetSpawnPosition(const AEVec2& spawnPos)
{
    position = spawnPos;
    RebuildTeleportBounds();

    teleportActive = false;
    teleportTimer = 0.f;
    teleportMoved = false;
    teleportCooldownTimer = 0.f;

    velocity = AEVec2{ 0.f, 0.f };
    chasing = false;
    facingDirection = AEVec2{ 1.f, 0.f };

    sprite.SetState(IDLE, false, nullptr);
}

// Spawns "charge" particles around the boss that drift inward.
// Call every frame during SPELLCAST wind-up.
void EnemyBoss::SpawnSpellChargeVfx(float dt)
{
    // control density consistently across FPS
    static float acc = 0.f;
    const float spawnPerSec = 70.f;     // tune: 40..120
    acc += dt * spawnPerSec;

    int n = (int)acc;
    if (n <= 0) return;
    acc -= (float)n;

    // Use a COPY so you don't mess up your normal trail emitter
    ParticleSystem::EmitterSettings e = particleSystem.emitter;

    // Tune for "slow drift"
    e.speedRange = { 1.2f, 2.6f };
    e.lifetimeRange = { 0.35f, 0.75f };
    e.tint = { 0.63f, 0.13f, 0.94f, 1.0f };
	e.behavior = ParticleBehavior::Inward;

    // Boss "visual center" (you already discovered you need +0.5f on X)
    const float cx = position.x + 0.5f;
    const float cy = position.y + 0.35f;

    // How far from the boss the particles start
    const float r = 0.5f;     // radius around boss (tune)
    const float band = 1.f; // thickness of the spawn band

    // split n into 4 directions
    int perSide = (n / 4);
    if (perSide < 1) perSide = 1;

    // LEFT side -> move RIGHT (angle ~ 0 deg)
    e.spawnPosRangeX = { cx - r - band, cx - r + band };
    e.spawnPosRangeY = { cy - r,        cy + r };
    e.angleRange = { AEDegToRad(-15.f), AEDegToRad(15.f) };
    particleSystem.SpawnParticleBurst(e, perSide);

    // RIGHT side -> move LEFT (angle ~ 180 deg)
    e.spawnPosRangeX = { cx + r - band, cx + r + band };
    e.spawnPosRangeY = { cy - r,        cy + r };
    e.angleRange = { AEDegToRad(165.f), AEDegToRad(195.f) };
    particleSystem.SpawnParticleBurst(e, perSide);

    // TOP side -> move DOWN (angle ~ 270 deg)
    e.spawnPosRangeX = { cx - r,        cx + r };
    e.spawnPosRangeY = { cy + r - band, cy + r + band };
    e.angleRange = { AEDegToRad(255.f), AEDegToRad(285.f) };
    particleSystem.SpawnParticleBurst(e, perSide);

    // BOTTOM side -> move UP (angle ~ 90 deg)
    e.spawnPosRangeX = { cx - r,        cx + r };
    e.spawnPosRangeY = { cy - r - band, cy - r + band };
    e.angleRange = { AEDegToRad(75.f), AEDegToRad(105.f) };
    particleSystem.SpawnParticleBurst(e, perSide);

}

//helper function to build teleport range base on boss spawn location/point
void EnemyBoss::RebuildTeleportBounds()
{
    spawnAnchorX = position.x;

    const float bossHalfW = size.x * 0.5f;

    teleportMinX = spawnAnchorX - teleportHalfRange + bossHalfW + teleportWallPadding;
    teleportMaxX = spawnAnchorX + teleportHalfRange - bossHalfW - teleportWallPadding;

    if (teleportMinX > teleportMaxX)
        std::swap(teleportMinX, teleportMaxX);
}

bool EnemyBoss::IsTeleportXValid(float targetX,
    const AEVec2& playerPos,
    MapGrid& map,
    bool allowOnPlayer) const
{
    if (targetX < teleportMinX || targetX > teleportMaxX)
        return false;

    if (!allowOnPlayer && std::fabs(targetX - playerPos.x) < teleportMinPlayerGap)
        return false;

    AEVec2 testPos = position;
    testPos.x = targetX;

    AEVec2 testSize = size;
    testSize.x = max(0.05f, testSize.x - teleportWallPadding);
    testSize.y = max(0.05f, testSize.y - teleportWallPadding);

    if (map.CheckBoxCollision(testPos, testSize))
        return false;

    return true;
}

float EnemyBoss::FindTeleportTarget(const AEVec2& playerPos,
    bool playerFacingRight,
    MapGrid& map) const
{
    const float behindDir = playerFacingRight ? -1.0f : 1.0f;

    const float d1 = teleportBehindOffset;
    const float d2 = teleportBehindOffset * 0.75f;
    const float d3 = teleportBehindOffset * 0.50f;

    const float behindCandidates[] =
    {
        playerPos.x + behindDir * d1,
        playerPos.x + behindDir * d2,
        playerPos.x + behindDir * d3
    };

    // Prefer valid positions behind the player only
    for (float x : behindCandidates)
    {
        if (IsTeleportXValid(x, playerPos, map))
            return x;
    }

    // Then try a clamped behind position, still behind-only
    const float clampedBehind =
        std::clamp(playerPos.x + behindDir * d1, teleportMinX, teleportMaxX);

    if (IsTeleportXValid(clampedBehind, playerPos, map))
        return clampedBehind;

    // Final fallback: teleport on the player if behind is blocked by wall/space
    /*const float onPlayerX =
        std::clamp(playerPos.x, teleportMinX, teleportMaxX);*/

    if (IsTeleportXValid(playerPos.x, playerPos, map, true))
        return playerPos.x;

    return position.x;
}

void EnemyBoss::Update(const AEVec2& playerPos, bool playerFacingRight, MapGrid& map)
{

	float dt = (float)AEFrameRateControllerGetFrameTime();

    auto UpdateBossParticles = [&]()
        {
            // Visual center (keep your known +0.5f X offset)
            const float cx = position.x + 0.5f;
            const float cy = position.y + 0.45f;   // roughly chest height, tune

            // --- AURA EMITTER (spawns around boss, not behind velocity) ---
            particleSystem.emitter.behavior = ParticleBehavior::normal;
      
            // Center slightly ABOVE boss so it "spirals upward"
            particleSystem.emitter.behaviorParams.center = { cx, cy + 0.8f };

            // Start values
            particleSystem.emitter.behaviorParams.swirl = 20.f;  
            particleSystem.emitter.behaviorParams.pull = 15.f;   

            // Spawn volume around the boss body
            AEVec2Set(&particleSystem.emitter.spawnPosRangeX, cx - 0.65f, cx + 0.65f);
            AEVec2Set(&particleSystem.emitter.spawnPosRangeY, position.y + 0.05f, position.y + 1.25f);

            // Give a gentle UP drift (so it looks like energy rising)
            particleSystem.emitter.angleRange.x = AEDegToRad(70.f);
            particleSystem.emitter.angleRange.y = AEDegToRad(110.f);

            // Keep initial speed low so behavior is visible
            particleSystem.emitter.speedRange.x = 0.5f;
            particleSystem.emitter.speedRange.y = 0.8f;

            particleSystem.emitter.lifetimeRange.x = 0.7f;
            particleSystem.emitter.lifetimeRange.y = 1.3f;

            // Intimidating color (purple aura).
            particleSystem.emitter.tint = { 0.65f, 0.15f, 0.95f, 0.75f };
            //particleSystem.emitter.tint = { 0.18f, 0.09f, 0.20f, 0.8f };

            // Spawn even when idle; optionally increase when moving
            const float speed = AEVec2Length(&velocity);
            particleSystem.SetSpawnRate(20.f + speed * 12.f); // tune: 100..220 base

            particleSystem.Update(); // keep if you want UpdateBossParticles to own the update
        };


    float hpTarget = (maxHP > 0) ? (float)hp / (float)maxHP : 0.f;
    hpTarget = max(0.f, min(1.f, hpTarget));

    const float hpRatio = hpTarget;          // 1.0 at full HP, 0.0 at death

    if (!phase2 && hpRatio <= phase2HpThreshold)
    {
        phase2 = true;
        SpecialElapsed = 0.0f;          // start phase 2 cooldown fresh
        specialBurstActive = false;
        specialSpawnsRemaining = 0;
        specialSpawnTimer = 0.0f;
    }
    const float pressure = 1.0f - hpRatio;   // 0.0 at full HP, 1.0 near death

    // Runtime tuning derived from HP
    const float runtimeTeleportInterval = Lerp(2.2f, 1.0f, pressure);
    const float runtimeTeleportSnapNorm = Lerp(0.28f, 0.10f, pressure);
    const float runtimeSpecialCooldown = Lerp(5.0f, 3.2f, pressure);
    const float runtimeSpecialSpawnGap = Lerp(1.0f, 0.55f, pressure);
    const float runtimeProjectileSpeed = Lerp(7.0f, 9.0f, pressure);
    const float runtimeMoveSpeed = Lerp(moveSpeed, moveSpeed * 1.12f, pressure);
    const float runtimeMeleeHitTimeNorm = Lerp(0.72f, 0.45f, pressure);

    attack.hitTimeNormalized = runtimeMeleeHitTimeNorm;

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
            SpawnSpellChargeVfx(dt);
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

    //const float dy = playerPos.y - position.y;
    const float YaggroRange = 5.f;
    const float dy = std::fabs(playerPos.y - position.y);
    const float absDy = std::fabs(dy);
    //const bool yAggroOk = (dy <= aggroYRange);
    const bool yAttackOk = (dy <= attackYRange);

    const bool inAggroRange = (absDx <= aggroRange && absDy <= YaggroRange);
    const float attackDur = GetAnimDurationSec(sprite, ATTACK);
    const float effectiveDist = yAttackOk ? absDx : 9999.0f;

    if (inAggroRange)
    {
        if (!bossEngaged)
            bossEngaged = true;

        // Only trigger HUD intro the first time it ever appears
        if (!bossHudVisible)
        {
            bossHudVisible = true;
            hudIntroStarted = true;
            hudIntroTimer = 0.0f;
        }
    }
    else
    {
        // Boss can disengage logically, but HUD stays visible
        bossEngaged = false;
    }

    if (hudIntroStarted)
    {
        hudIntroTimer += dt;
    }

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
        const float snapTime = (teleDur > 0.f) ? teleDur * runtimeTeleportSnapNorm : 0.f;

        // Snap behind player once, mid-animation
        if (!teleportMoved && teleportTimer >= snapTime)
        {
            const float targetX = FindTeleportTarget(playerPos, playerFacingRight, map);

            position.x = targetX;

            facingDirection = AEVec2{ (playerPos.x >= position.x) ? 1.f : -1.f, 0.f };

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
            //const float newDx = playerPos.x - position.x;
            //const float newAbsDx = std::fabs(newDx);
            //const float attackDur = GetAnimDurationSec(sprite, ATTACK);

            attack.Reset();                 // clears cooldownTimer too
           // const float effectiveDist = yAttackOk ? absDx : 9999.0f;

            const float postDx = std::fabs(playerPos.x - position.x);
            const float postDy = std::fabs(playerPos.y - position.y);
            const bool postYAttackOk = (postDy <= attackYRange);
            const float postEffectiveDist = postYAttackOk ? postDx : 9999.0f;

            attack.Update(dt, postEffectiveDist, attackDur);
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
    if (inAggroRange)
    {
        teleportCooldownTimer += dt;

        const bool canStartTeleport =
            !attack.IsAttacking() &&
            !teleportBlockedBySpecial &&
            !specialBurstActive &&
            !teleportActive;

        if (canStartTeleport && teleportCooldownTimer >= runtimeTeleportInterval)
        {
            teleportActive = true;
            teleportTimer = 0.f;
            teleportMoved = false;
            teleportCooldownTimer = 0.f;

            sprite.SetState(TELEPORT);
            attack.Reset();
            chasing = false;
            velocity = AEVec2{ 0.f, 0.f };
        }
    }
    else
    {
        teleportCooldownTimer = 0.f;
    }


    // Update attack component
    attack.Update(dt, effectiveDist, attackDur);

    // --- Special sequence ---
    //static constexpr float kSpecialCooldown = 5.0f;
    //static constexpr float kSpecialSpawnGap = 1.0f;   // 0.5s between spawns
    static constexpr int   kSpecialSpawnCount = 5;

    if (phase2)
    {
        if (specialBurstActive)
        {
            SpawnSpellChargeVfx(dt);
            // Face player during special
            if (dx != 0.f)
                facingDirection = AEVec2{ (dx > 0.f) ? 1.f : -1.f, 0.f };

            // >>> ADD THIS BLOCK HERE <<<
            /*if (specialSpawnsRemaining == kSpecialSpawnCount && specialSpawnTimer > 0.f)
            {
                SpawnSpellChargeVfx(dt);
            }*/


            // Timer handles: initial cast delay + spawn gaps
       
            specialSpawnTimer -= dt;

            while (specialSpawnTimer <= 0.0f && specialSpawnsRemaining > 0)
            {
                const float dir = (facingDirection.x >= 0.f) ? 1.f : -1.f;

                SpecialAttack specialAttack;
                specialAttack.pos = AEVec2{ position.x + dir * 0.6f, position.y + 0.35f };
                specialAttack.vel = AEVec2{ dir * runtimeProjectileSpeed, 0.f };
                specialAttack.radius = 0.14f;
                specialAttack.life = 1.6f;
                specialAttack.color = 0xFFAA66FF;

                g_specialAttacks.push_back(specialAttack);
                SpawnSpecialMuzzleBurst(specialAttack.pos, dir);
                --specialSpawnsRemaining;
                if (specialSpawnsRemaining <= 0) g_spellcastUntil5thSpawn = false; // ? stop spellcast as soon as 5th is spawned

                specialSpawnTimer += runtimeSpecialSpawnGap;
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

            if (SpecialElapsed >= runtimeSpecialCooldown)
            {
                // Start special
                specialBurstActive = true;
                specialSpawnsRemaining = kSpecialSpawnCount;
                SpecialElapsed = 0.0f;
                g_spellcastUntil5thSpawn = true;

                static constexpr float kChargeUpExtra = 0.67f;
                // Start SPELLCAST now
                sprite.SetState(SPELLCAST);

                // IMPORTANT CHANGE:
                // Start spawning on the *last frame start* of SPELLCAST so there's no visible "dead gap"
                const float castDur = GetAnimDurationSec(sprite, SPELLCAST);
                const float tpf = GetAnimTimePerFrame(sprite, SPELLCAST);
                specialSpawnTimer = ((castDur > 0.f) ? max(0.0f, castDur - tpf) : 0.0f) + kChargeUpExtra;
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
                velocity.x = dirX * runtimeMoveSpeed;
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
            UpdateBossParticles();
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
    /*for (auto& specialAttack : g_specialAttacks)
        specialAttack.Update(dt);

    g_specialAttacks.erase(
        std::remove_if(g_specialAttacks.begin(), g_specialAttacks.end(),
            [](const SpecialAttack& specialAttack) { return !specialAttack.alive(); }),
        g_specialAttacks.end()
    );*/

    for (auto& specialAttack : g_specialAttacks)
    {
        specialAttack.Update(dt);
        if (specialAttack.alive())
            SpawnSpecialTrail(specialAttack);
    }

    g_specialAttacks.erase(
        std::remove_if(g_specialAttacks.begin(), g_specialAttacks.end(),
            [&](const SpecialAttack& specialAttack)
            {
                if (!specialAttack.alive())
                {
                    SpawnSpecialImpactBurst(specialAttack.pos);
                    return true;
                }
                return false;
            }),
        g_specialAttacks.end()
    );


    UpdateMeleeHitbox(playerPos);

    if (!isDead) UpdateBossParticles();
}

void EnemyBoss::SpawnImpactBurst()
{
    // Use the SAME visual X offset already use in UpdateBossParticles()
    const float visualX = position.x + 0.5f;

    // "Feet" approx (hurtbox is 0.8 tall, tune if needed)
    const float feetY = position.y - (size.y * 0.5f);

    // In front of boss depending on facing
    const float dirX = (facingDirection.x >= 0.f) ? 1.f : -1.f;
    const float hitX = visualX + dirX * (size.x * 0.55f);

    // use CUSTOM emitters
    ParticleSystem::EmitterSettings e = particleSystem.emitter;
    e.behavior = ParticleBehavior::Gravity;
    e.tint = { 0.8f, 0.4f, 0.2f, 1.0f }; // fiery orange; tune as needed


    // Layer 1: Ground shock (left + right spray)
   
    e.spawnPosRangeX = { hitX - 0.05f, hitX + 0.05f };
    e.spawnPosRangeY = { feetY - 0.05f, feetY + 0.10f };
    e.speedRange = { 14.f, 24.f };
    e.lifetimeRange = { 0.08f, 0.14f };

      //right spray
    e.angleRange = { AEDegToRad(-20.f), AEDegToRad(20.f) };
    particleSystem.SpawnParticleBurst(e, 18);

    // left spray
    e.angleRange = { AEDegToRad(160.f), AEDegToRad(200.f) };
    particleSystem.SpawnParticleBurst(e, 18);

  
    // Layer 2: Debris cone upward
    e.spawnPosRangeX = { hitX - 0.18f, hitX + 0.18f };
    e.spawnPosRangeY = { feetY + 0.00f, feetY + 0.20f };
    e.speedRange = { 18.f, 40.f };
    e.lifetimeRange = { 0.20f, 0.55f };
    e.angleRange = { AEDegToRad(70.f), AEDegToRad(110.f) };
    particleSystem.SpawnParticleBurst(e, 22);

    // =========================
    // Layer 3: Puff / flash
    // =========================
    e.spawnPosRangeX = { hitX - 0.12f, hitX + 0.12f };
    e.spawnPosRangeY = { feetY + 0.10f, feetY + 0.40f };
    e.speedRange = { 6.f, 12.f };
    e.lifetimeRange = { 0.05f, 0.12f };
    e.angleRange = { AEDegToRad(0.f), AEDegToRad(360.f) };
    particleSystem.SpawnParticleBurst(e, 12);
}

void EnemyBoss::SpawnSpecialTrail(const SpecialAttack& s)
{
    ParticleSystem::EmitterSettings e = particleSystem.emitter;


    e.behavior = ParticleBehavior::Gravity;
    e.tint = { 0.70f, 0.30f, 1.00f, 0.85f };

    e.lifetimeRange = { 0.10f, 0.22f };
    e.speedRange = { 0.2f, 0.8f };

    const float trailLen = 0.4f;
    const bool faceRight = (s.vel.x >= 0.0f);

    if (faceRight)
        e.spawnPosRangeX = { s.pos.x - trailLen, s.pos.x + 0.5f };
    else
        e.spawnPosRangeX = { s.pos.x - 0.5f, s.pos.x + trailLen };

    e.spawnPosRangeY = { s.pos.y - 0.5f, s.pos.y + 1.5f };
    e.angleRange = { AEDegToRad(0.f), AEDegToRad(360.f) };

    particleSystem.SpawnParticleBurst(e, 2);
}

void EnemyBoss::SpawnSpecialImpactBurst(const AEVec2& hitPos)
{
    ParticleSystem::EmitterSettings e = particleSystem.emitter;

    e.behavior = ParticleBehavior::Gravity;
    e.tint = { 0.85f, 0.45f, 1.00f, 1.0f };

    // flash
    e.spawnPosRangeX = { hitPos.x - 0.05f, hitPos.x + 0.05f };
    e.spawnPosRangeY = { hitPos.y - 0.05f, hitPos.y + 0.05f };
    e.speedRange = { 4.f, 10.f };
    e.lifetimeRange = { 0.06f, 0.12f };
    e.angleRange = { AEDegToRad(0.f), AEDegToRad(360.f) };
    particleSystem.SpawnParticleBurst(e, 10);

    // outward shards
    e.spawnPosRangeX = { hitPos.x - 0.08f, hitPos.x + 0.08f };
    e.spawnPosRangeY = { hitPos.y - 0.08f, hitPos.y + 0.08f };
    e.speedRange = { 8.f, 18.f };
    e.lifetimeRange = { 0.15f, 0.35f };
    e.angleRange = { AEDegToRad(0.f), AEDegToRad(360.f) };
    particleSystem.SpawnParticleBurst(e, 16);
}

void EnemyBoss::SpawnSpecialMuzzleBurst(const AEVec2& spawnPos, float dir)
{
    ParticleSystem::EmitterSettings e = particleSystem.emitter;
    e.tint = { 0.85f, 0.45f, 1.00f, 1.0f };

    e.behavior = ParticleBehavior::normal;
    e.tint = { 0.72f, 0.28f, 1.00f, 1.0f };

    e.spawnPosRangeX = { spawnPos.x - 0.05f, spawnPos.x + 0.05f };
    e.spawnPosRangeY = { spawnPos.y - 0.05f, spawnPos.y + 0.05f };
    e.speedRange = { 6.f, 14.f };
    e.lifetimeRange = { 0.10f, 0.22f };

    if (dir >= 0.f)
        e.angleRange = { AEDegToRad(-25.f), AEDegToRad(25.f) };
    else
        e.angleRange = { AEDegToRad(155.f), AEDegToRad(205.f) };

    particleSystem.SpawnParticleBurst(e, 14);
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
            SpawnSpecialImpactBurst(s.pos);
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
    particleSystem.Render();
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

        if (attack.IsAttacking())
        {
            const auto& hb = meleeHitbox;
            QuickGraphics::DrawRect(hb.position.x, hb.position.y, hb.size.x, hb.size.y,
                0xFF00FFFF, AE_GFX_MDM_LINES_STRIP);
        }
      
    }
    RenderHealthbar();

    // make sure we�re not stuck in additive mode from VFX
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
}

void EnemyBoss::Reset(const AEVec2& spawnPos)
{
    position = spawnPos;
    RebuildTeleportBounds();
    velocity = AEVec2{ 0.f, 0.f };
    facingDirection = AEVec2{ 1.f, 0.f };

    hp = maxHP;
    isDead = false;
    hideAfterDeath = false;

    chasing = false;
    isAttacking = false;
    isGrounded = false;

    deathTimeLeft = 0.f;
    hurtTimeLeft = 0.f;
    invulnTimer = 0.f;

    attack.Reset();


    teleportActive = false;
    teleportTimer = 0.f;
    teleportMoved = false;
    teleportCooldownTimer = 0.f;

    specialUnlocked = false;
    SpecialElapsed = 0.f;
    specialBurstActive = false;
    specialSpawnsRemaining = 0;
    specialSpawnTimer = 0.f;

    g_spellcastUntil5thSpawn = false;
    g_specialAttacks.clear();

    bossHudVisible = false;
    bossEngaged = false;
    hudIntroStarted = false;
    hudIntroTimer = 0.f;

    hpBarFront = 1.f;
    hpBarChip = 1.f;
    prevHpTarget = 1.f;
    hpChipDelay = 0.f;

    sprite.SetState(IDLE, false, nullptr);
    specialAttackVfx.SetState(SPELL1);

    particleSystem.SetSpawnRate(0.f);
}

void EnemyBoss::DrawInspector()
{
    ImGui::Begin("EnemyBoss", &isInspectorOpen);

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
    if (!bossHudVisible) return;

    float t = hudIntroStarted ? hudIntroTimer : 999.0f;

    float nameAlpha = t / nameFadeDuration;
    nameAlpha = max(0.0f, min(1.0f, nameAlpha));

    float barReveal = (t - barStartDelay) / barFillDuration;
    barReveal = max(0.0f, min(1.0f, barReveal));
      
    AEMtx33 ui;
    AEMtx33Scale(&ui, 1.f, 1.f);
    AEGfxSetTransform(ui.m);

    // --- HUD anchor in screen pixels ---
    const float screenW = (float)AEGfxGetWindowWidth();

    const float topMarginPx = 70.f;   // distance from top of screen
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

    float chipW = barW * hpBarChip * barReveal;
    float frontW = barW * hpBarFront * barReveal;

    float chipCenterX = barLeftX + chipW * 0.5f;
    float frontCenterX = barLeftX + frontW * 0.5f;
    AEGfxSetBlendMode(AE_GFX_BM_BLEND); // safety

    u32 bgColor = ScaleAlpha(0xFFFFFFFF, barReveal);
    u32 borderColor = ScaleAlpha(0xFFFFFFFF, barReveal);
    u32 chipColor = ScaleAlpha(0xFF7A0000, barReveal);
    u32 frontColor = ScaleAlpha(0xFFFF0000, barReveal);


    //background and border of health bar
    QuickGraphics::DrawRect(barCenterX, barY, barW, barH, bgColor, AE_GFX_MDM_TRIANGLES);
    QuickGraphics::DrawRect(barCenterX, barY, barW, barH, borderColor, AE_GFX_MDM_LINES_STRIP);

    if (frontW > 0.001f)
    {
        QuickGraphics::DrawRect(chipCenterX, barY, chipW, barH, chipColor, AE_GFX_MDM_TRIANGLES);
        QuickGraphics::DrawRect(frontCenterX, barY, frontW, barH, frontColor, AE_GFX_MDM_TRIANGLES);
    }
   

    // Text (optional)
   // (Top-left label + HP numbers)
    std::string label = "Bringer of Death";
  
    //QuickGraphics::PrintText(label.c_str(), -0.15f, 0.88f, 0.35f, 1, 1, 1, nameAlpha);
    AEGfxPrint(bossFont, label.c_str(), -0.18f, 0.88f, 0.50f, 1.0f, 1.0f, 1.0f, nameAlpha);
    /*
    std::string hpStr = std::to_string(hp) + " / " + std::to_string(maxHP);
    QuickGraphics::PrintText(hpStr.c_str(), 0.55f, 0.88f, 0.35f, 1, 1, 1, 1);
    */
}
