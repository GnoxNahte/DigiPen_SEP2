#include "traps.h"
#include "../Camera.h"

#include <algorithm>
#include <iostream>


#include "../../Game/Player/Player.h"
#include "../../Utils/QuickGraphics.h"

// ---------- AABB overlap ----------
static inline float MinX(const Box& b) { return b.position.x; }
static inline float MinY(const Box& b) { return b.position.y; }
static inline float MaxX(const Box& b) { return b.position.x + b.size.x; }
static inline float MaxY(const Box& b) { return b.position.y + b.size.y; }

AEGfxTexture* SpikePlate::s_spikeTexture = nullptr;
AEGfxVertexList* SpikePlate::s_spikeMeshes[4] = { nullptr, nullptr, nullptr, nullptr };
bool SpikePlate::s_resourcesLoaded = false;

bool IntersectsBox(const Box& a, const Box& b)
{
    const float eps = 0.0001f;

    if (MaxX(a) <= MinX(b) + eps) return false;
    if (MinX(a) >= MaxX(b) - eps) return false;
    if (MaxY(a) <= MinY(b) + eps) return false;
    if (MinY(a) >= MaxY(b) - eps) return false;
    return true;
}

Box MakePlayerFeetBox(const Player& p)
{
    Box b{};
    const AEVec2 fullSize = p.GetStats().playerSize;
	const AEVec2 pos = p.GetPosition(); // pos is the center of the player

	// width covers the whole player to make it easier to step on plates, and also visually looks better since the plate is under the player's feet, so it makes sense for the box to be as wide as the player. The height is just a thin strip at the bottom of the player to check for stepping on plates, and it also visually looks better for spikes to come out of the ground between the player's feet instead of right in the middle of the player.
    b.size.x = fullSize.x * 0.4f;;
	// add on the y axis to cover the area where the player's feet would be, so that player can still trigger pressure plates even if they are slightly above it due to the groundChecker intercepting a bit early, and it also visually looks better since the player is stepping on the plate with their feet instead of their body. The height of 0.4f is chosen to be large enough to account for the groundChecker intercepting a bit early and causing the player to be slightly above the plate, but not too large that it would cause the plate to be triggered when the player is just near it.
    b.size.y = 0.4f;

	// X axis : player center minus half the width to align with left edge of player, so that the plate is under the player's feet and it's easier to step on plates since the box is as wide as the player, and it also visually looks better since the plate is under the player's feet, so it makes sense for the box to be as wide as the player
    b.position.x = pos.x - (b.size.x * 0.5f);
	// Y axis : player center minus half the player's height to get to the bottom of the player, then add a small tolerance of 0.25f to account for the groundChecker intercepting a bit early and causing the player to be slightly above the plate, and it also visually looks better since the player is stepping on the plate with their feet instead of their body. The tolerance of 0.25f is chosen to be large enough to account for the groundChecker intercepting a bit early and causing the player to be slightly above the plate, but not too large that it would cause the plate to be triggered when the player is just near it.
	// the 0.5f is to get to the bottom of the player, since the player's position is at the center, and we want the box to be at the feet. Then we add 0.25f to give some tolerance for the player being slightly above the plate due to the groundChecker intercepting a bit early, and it also visually looks better since the player is stepping on the plate with their feet instead of their body.
    b.position.y = pos.y - (fullSize.y * 0.5f) - 0.25f;

    return b;
}

Box MakePlayerBodyBox(const Player& p)
{
    Box b{};
    const AEVec2 fullSize = p.GetStats().playerSize;
    const AEVec2 pos = p.GetPosition();

	// traps that damage on touch should use this box, which covers most of the player's body but not the feet, so that player can still step on pressure plates without taking damage, and it also won't accidentally trigger when player is just near the trap but not actually touching it
	b.size.x = fullSize.x * 0.1f;   // wider than hurtbox to be more forgiving for trap hits, but not too wide to accidentally trigger when just near
	b.size.y = fullSize.y * 0.55f;   // cover most of the body but not the feet, so player can still step on plates without taking damage, and it also visually looks better for spikes to come out of the ground between the player's feet instead of right in the middle of the player

    b.position.x = pos.x - b.size.x * 0.5f;
    b.position.y = pos.y - (b.size.y * 0.5f);

    return b;
}

AEGfxVertexList* SpikePlate::MakeSpikeMesh(int frame)
{
    const float uMin = frame * 0.25f;
    const float uMax = (frame + 1) * 0.25f;

    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, uMin, 1.f,
        0.5f, -0.5f, 0xFFFFFFFF, uMax, 1.f,
        -0.5f, 0.5f, 0xFFFFFFFF, uMin, 0.f);

    AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, uMax, 1.f,
        0.5f, 0.5f, 0xFFFFFFFF, uMax, 0.f,
        -0.5f, 0.5f, 0xFFFFFFFF, uMin, 0.f);

    return AEGfxMeshEnd();
}

void SpikePlate::LoadSharedRenderResources()
{
    if (s_resourcesLoaded)
        return;

    s_spikeTexture = AEGfxTextureLoad("Assets/Tmp/spikes.png");
    for (int i = 0; i < 4; ++i)
        s_spikeMeshes[i] = MakeSpikeMesh(i);

    s_resourcesLoaded = true;
}

void SpikePlate::UnloadSharedRenderResources()
{
    if (!s_resourcesLoaded)
        return;

    if (s_spikeTexture)
    {
        AEGfxTextureUnload(s_spikeTexture);
        s_spikeTexture = nullptr;
    }

    for (int i = 0; i < 4; ++i)
    {
        if (s_spikeMeshes[i])
        {
            AEGfxMeshFree(s_spikeMeshes[i]);
            s_spikeMeshes[i] = nullptr;
        }
    }

    s_resourcesLoaded = false;
}

// ---------------- Trap ----------------
Trap::Trap(Type type, const Box& box) : m_type(type), m_box(box) {}

void Trap::Update(float dt, Player& player)
{
    if (!m_enabled) { m_prevOverlap = false; return; }

    Box playerBox;
    if (m_type == Type::PressurePlate || m_type == Type::LavaPool)
    {
        playerBox = MakePlayerFeetBox(player);
    }
    else
    {
        playerBox = MakePlayerBodyBox(player);
    }


    const bool overlap = IntersectsBox(m_box, playerBox);

    if (overlap && !m_prevOverlap) OnPlayerEnter(player);
    if (overlap)                   OnPlayerStay(dt, player);
    if (!overlap && m_prevOverlap) OnPlayerExit(player);

    m_prevOverlap = overlap;
}

void Trap::Render() const
{
    unsigned int color = 0xFFFFFFFF;
    switch (m_type)
    {
    case Type::LavaPool:      color = 0xFFFF5500; break;
    case Type::PressurePlate: color = 0xFF00FF00; break;
    case Type::SpikePlate:    color = 0xFFAAAAAA; break;
    }

   // QuickGraphics::DrawRect(m_box.position, m_box.size, color, AE_GFX_MDM_TRIANGLES);

}



// ---------------- LavaPool ----------------
LavaPool::LavaPool(const Box& box, int damagePerTick, float tickInterval)
    : Trap(Type::LavaPool, box),
    m_damagePerTick((std::max)(1, damagePerTick)),
    m_tickInterval((std::max)(0.01f, tickInterval))
{
}

void LavaPool::OnPlayerEnter(Player& player)
{
    AEVec2 trapOrigin = { player.GetPosition().x, player.GetPosition().y - 1.0f };
    player.TryTakeDamage(m_damagePerTick, trapOrigin);

	// entering lava should cause immediate damage, and then start the tick timer so that it will deal damage periodically after that as well
    m_tickTimer = 0.f;
}

void LavaPool::OnPlayerStay(float dt, Player& player)
{
    m_tickTimer += dt;
    if (m_tickTimer >= m_tickInterval)
    {
        m_tickTimer -= m_tickInterval;
        AEVec2 trapOrigin = { player.GetPosition().x, player.GetPosition().y - 1.0f };
        player.TryTakeDamage(m_damagePerTick, trapOrigin);
    }
}

// ---------------- PressurePlate ----------------
PressurePlate::PressurePlate(const Box& box) : Trap(Type::PressurePlate, box) {}

void PressurePlate::AddLinkedTrap(Trap* t)
{
    if (!t) return;

    if (std::find(m_linked.begin(), m_linked.end(), t) != m_linked.end())
        return;

    m_linked.push_back(t);
}

void PressurePlate::OnPlayerEnter(Player&)
{
    if (IsTriggered()) 
    { 
        return;
    }
    std::cout << "[Plate] Triggered!\n";
    MarkTriggered();
    for (Trap* t : m_linked)
    {
        if (!t) continue;

        if (auto* spike = dynamic_cast<SpikePlate*>(t))
			spike->ActivateFromPlate();   // raise spikes immediately if linked to a spike plate
        else
            t->SetEnabled(true);
    }
}

// ---------------- SpikePlate ----------------
SpikePlate::SpikePlate(const Box& box, float upTime, float downTime, int damageOnHit, bool startDisabled)
    : Trap(Type::SpikePlate, box),
    m_upTime((std::max)(0.05f, upTime)),
    m_downTime((std::max)(0.05f, downTime)),
    m_damageOnHit((std::max)(1, damageOnHit))
{
    SetEnabled(!startDisabled);

    if (IsEnabled())
    {
        m_spikesUp = true;
        m_phaseTimer = 0.f;
        m_animFrame = 3;   // always on
    }
    else
    {
        m_spikesUp = false;
        m_phaseTimer = 0.f;
        m_animFrame = 0;   // ini off
    }

    m_animTimer = 0.f;
}

void SpikePlate::ActivateFromPlate()
{
    SetEnabled(true);
    m_spikesUp = true;
    m_lockedOn = true;
    m_phaseTimer = 0.f;
    m_hitTimer = 0.f;
    m_animTimer = 0.f;
}

void SpikePlate::Update(float dt, Player& player)
{
    if (!IsEnabled())
    {
        m_spikesUp = false;
        m_animFrame = 0;
        m_animTimer = 0.f;
        return;
    }

    // cooldown
    if (m_hitTimer > 0.f)
        m_hitTimer = (std::max)(0.f, m_hitTimer - dt);

    // 逻辑状态机
    if (m_lockedOn)
    {
        m_spikesUp = true;
    }
    else
    {
        m_phaseTimer += dt;

        if (m_spikesUp)
        {
            if (m_phaseTimer >= m_upTime)
            {
                m_spikesUp = false;
                m_phaseTimer = 0.f;
            }
        }
        else
        {
            if (m_phaseTimer >= m_downTime)
            {
                m_spikesUp = true;
                m_phaseTimer = 0.f;
            }
        }
    }

    // ===== sprite animation =====
    m_animTimer += dt;
    while (m_animTimer >= 0.08f)
    {
        m_animTimer -= 0.08f;

        if (m_spikesUp)
        {
            if (m_animFrame < 3)
                ++m_animFrame;
        }
        else
        {
            if (m_animFrame > 0)
                --m_animFrame;
        }
    }

    Trap::Update(dt, player);
}

void SpikePlate::OnPlayerEnter(Player& player)
{
    if (!m_spikesUp) return;
    if (m_hitTimer > 0.f) return;

    std::cout << "[Spike] Enter Hit!\n";
    AEVec2 trapOrigin = { player.GetPosition().x, player.GetPosition().y - 1.0f };
    player.TryTakeDamage(m_damageOnHit, trapOrigin);

    m_hitTimer = m_hitCooldown;;
}

void SpikePlate::OnPlayerStay(float, Player& player)
{
    if (!m_spikesUp) return;
    if (m_hitTimer > 0.f) return;
    std::cout << "[Spike] Hit!\n";
    AEVec2 trapOrigin = { player.GetPosition().x, player.GetPosition().y - 1.0f };
    player.TryTakeDamage(m_damageOnHit, trapOrigin);

    m_hitTimer = m_hitCooldown;;
}


void SpikePlate::Render() const
{
    if (!s_resourcesLoaded || !s_spikeTexture)
        return;

    int frame = m_animFrame;
    if (frame < 0) frame = 0;
    if (frame > 3) frame = 3;

    const Box& box = GetBox();
    const float centerX = box.position.x + box.size.x * 0.5f;
    const float centerY = box.position.y + box.size.y * 0.5f;

    AEMtx33 m;
    AEMtx33Trans(&m, centerX, centerY);

    // using box size
    AEMtx33ScaleApply(&m, &m, box.size.x * Camera::scale, box.size.y * Camera::scale);

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1.f, 1.f, 1.f, 1.f);
    AEGfxSetColorToAdd(0.f, 0.f, 0.f, 0.f);
    AEGfxSetTransparency(1.f);
    AEGfxTextureSet(s_spikeTexture, 0.f, 0.f);
    AEGfxSetTransform(m.m);
    AEGfxMeshDraw(s_spikeMeshes[frame], AE_GFX_MDM_TRIANGLES);
}


// ---------------- TrapManager ----------------
void TrapManager::Update(float dt, Player& player)
{
    for (auto& t : m_traps) t->Update(dt, player);
}

void TrapManager::Render() const
{
    for (auto& t : m_traps) t->Render();
}



