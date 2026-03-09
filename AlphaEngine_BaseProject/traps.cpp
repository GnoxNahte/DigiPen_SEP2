#include "traps.h"

#include <algorithm>
#include <iostream>


#include "Source/Game/Player/Player.h"
#include "Source/Utils/QuickGraphics.h"

// ---------- AABB overlap ----------
static inline float MinX(const Box& b) { return b.position.x; }
static inline float MinY(const Box& b) { return b.position.y; }
static inline float MaxX(const Box& b) { return b.position.x + b.size.x; }
static inline float MaxY(const Box& b) { return b.position.y + b.size.y; }

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
    const AEVec2 pos = p.GetPosition();

	// for ground check and pressure plate triggering: a wide but very thin box at the player's feet, so that it can trigger when player is just barely stepping on it, but won't trigger if player is just near it
	b.size.x = fullSize.x * 0.6f;  // wider than hurtbox to be more forgiving for stepping on plates, but not too wide to accidentally trigger when just near
	b.size.y = 0.2f;               // very thin, just a sliver at the feet

    b.position.x = pos.x - b.size.x * 0.5f;
	// moving the box down so that it sits at the player's feet instead of centered on the player's position. The 0.05f is a small extra offset to make it more likely to trigger when stepping on plates.
    b.position.y = pos.y - (fullSize.y * 0.5f) - 0.05f;

    return b;
}

Box MakePlayerBodyBox(const Player& p)
{
    Box b{};
    const AEVec2 fullSize = p.GetStats().playerSize;
    const AEVec2 pos = p.GetPosition();

	// traps that damage on touch should use this box, which covers most of the player's body but not the feet, so that player can still step on pressure plates without taking damage, and it also won't accidentally trigger when player is just near the trap but not actually touching it
	b.size.x = fullSize.x * 0.8f;   // wider than hurtbox to be more forgiving for trap hits, but not too wide to accidentally trigger when just near
	b.size.y = fullSize.y * 0.9f;   // cover most of the body but not the feet, so player can still step on plates without taking damage, and it also visually looks better for spikes to come out of the ground between the player's feet instead of right in the middle of the player

    b.position.x = pos.x - b.size.x * 0.5f;
    b.position.y = pos.y - (b.size.y * 0.5f);

    return b;
}



// ---------------- Trap ----------------
Trap::Trap(Type type, const Box& box) : m_type(type), m_box(box) {}

void Trap::Update(float dt, Player& player)
{
    if (!m_enabled) { m_prevOverlap = false; return; }

    Box playerBox;
    if (m_type == Type::PressurePlate)
    {
		playerBox = MakePlayerFeetBox(player); // check feet for pressure plates so player can still trigger when just stepping on it, and won't trigger if player is just near it. Also, this makes it visually clearer that the player is stepping on the plate, since the box is at the feet instead of centered on the player's position.
    }
    else
    {
		playerBox = MakePlayerBodyBox(player); // checking body for damaging traps so player has to actually be touching the trap to take damage, and it also visually looks better for spikes to come out of the ground between the player's feet instead of right in the middle of the player. Pressure plates will use a separate feet box so that player can still trigger them without having to have their body hit the plate, which also allows for more forgiving triggering and visually looks better since the player is stepping on the plate with their feet instead of their body.
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
    if (t) m_linked.push_back(t);
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

	// if starting enabled, start with spikes up. otherwise, start with spikes down and timer at 0 so that it will switch to up after downTime.
    if (IsEnabled())
    {
        m_spikesUp = true;
        m_phaseTimer = 0.f;
    }

}

void SpikePlate::ActivateFromPlate()
{
    SetEnabled(true);
    m_spikesUp = true;
    m_lockedOn = true;
    m_phaseTimer = 0.f;
    m_hitTimer = 0.f;
}

void SpikePlate::Update(float dt, Player& player)
{
    if (!IsEnabled()) return;

	// update hit cooldown timer regardless of spike state, so that it will be ready to hit immediately when spikes come up
    if (m_hitTimer > 0.f)
        m_hitTimer = (std::max)(0.f, m_hitTimer - dt);

	// if locked on by pressure plate, stay up indefinitely and skip normal timing logic
    if (m_lockedOn)
    {
        m_spikesUp = true;
        Trap::Update(dt, player);
        return;
    }

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

    Trap::Update(dt, player);
}

void SpikePlate::OnPlayerEnter(Player& player)
{
    if (!m_spikesUp) return;
    if (m_hitTimer > 0.f) return;

    std::cout << "[Spike] Enter Hit!\n";
    AEVec2 trapOrigin = { player.GetPosition().x, player.GetPosition().y - 1.0f };
    player.TryTakeDamage(m_damageOnHit, trapOrigin);

    m_hitTimer = 0.5f;
}

void SpikePlate::OnPlayerStay(float, Player& player)
{
    if (!m_spikesUp) return;
    if (m_hitTimer > 0.f) return;
    std::cout << "[Spike] Hit!\n";
    AEVec2 trapOrigin = { player.GetPosition().x, player.GetPosition().y - 1.0f };
    player.TryTakeDamage(m_damageOnHit, trapOrigin);

    m_hitTimer = 0.5f;
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

