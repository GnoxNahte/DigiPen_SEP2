#include "traps.h"

#include <algorithm>
#include <iostream>


#include "Source/Game/Player/Player.h"
#include "Source/Utils/QuickGraphics.h"

// ---------- AABB overlap：按 position 为左下(或左上) + size ----------
static inline float MinX(const Box& b) { return b.position.x; }
static inline float MinY(const Box& b) { return b.position.y; }
static inline float MaxX(const Box& b) { return b.position.x + b.size.x; }
static inline float MaxY(const Box& b) { return b.position.y + b.size.y; }

bool IntersectsBox(const Box& a, const Box& b)
{
    if (MaxX(a) < MinX(b)) return false;
    if (MinX(a) > MaxX(b)) return false;
    if (MaxY(a) < MinY(b)) return false;
    if (MinY(a) > MaxY(b)) return false;
    return true;
}

Box MakePlayerBox(const Player& p)
{
    Box b{ };
    b.position = p.GetPosition();
    b.size = p.GetStats().playerSize;
    return b;
}

// ---------------- Trap ----------------
Trap::Trap(Type type, const Box& box) : m_type(type), m_box(box) {}

void Trap::Update(float dt, Player& player)
{
    if (!m_enabled) { m_prevOverlap = false; return; }

    const Box playerBox = MakePlayerBox(player);
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

    QuickGraphics::DrawRect(m_box.position, m_box.size, color, AE_GFX_MDM_TRIANGLES);

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
        player.TakeDamage(m_damagePerTick);
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
    for (Trap* t : m_linked) if (t) t->SetEnabled(true);
}

// ---------------- SpikePlate ----------------
SpikePlate::SpikePlate(const Box& box, float upTime, float downTime, int damageOnHit, bool startDisabled)
    : Trap(Type::SpikePlate, box),
    m_upTime((std::max)(0.05f, upTime)),
    m_downTime((std::max)(0.05f, downTime)),
    m_damageOnHit((std::max)(1, damageOnHit))
{
    SetEnabled(!startDisabled);
}

void SpikePlate::Update(float dt, Player& player)
{
    if (!IsEnabled()) return;

    m_phaseTimer += dt;
    if (m_spikesUp)
    {
        if (m_phaseTimer >= m_upTime) { m_spikesUp = false; m_phaseTimer = 0.f; }
    }
    else
    {
        if (m_phaseTimer >= m_downTime) { m_spikesUp = true; m_phaseTimer = 0.f; }
    }

    if (m_hitTimer > 0.f) m_hitTimer = (std::max)(0.f, m_hitTimer - dt);

    Trap::Update(dt, player);
}

void SpikePlate::OnPlayerStay(float, Player& player)
{
    if (!m_spikesUp) return;
    if (m_hitTimer > 0.f) return;
    std::cout << "[Spike] Hit!\n";
    player.TakeDamage(m_damageOnHit);
    m_hitTimer = m_hitCooldown;
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
