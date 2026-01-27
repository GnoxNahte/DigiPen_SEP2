#pragma once
#include <vector>
#include <memory>

#include "AEEngine.h"
#include "Source/Utils/Box.h" 

class Player;

bool IntersectsBox(const Box& a, const Box& b);
Box MakePlayerBox(const Player& p);

class Trap
{
public:
    enum class Type { LavaPool, PressurePlate, SpikePlate };

    Trap(Type type, const Box& box);
    virtual ~Trap() = default;

    virtual void Update(float dt, Player& player);
    virtual void Render() const;

    void SetEnabled(bool e) { m_enabled = e; }
    bool IsEnabled() const { return m_enabled; }

    bool IsTriggered() const { return m_triggered; }
    void MarkTriggered() { m_triggered = true; }

    const Box& GetBox() const { return m_box; }
    void SetBox(const Box& b) { m_box = b; }

protected:
    virtual void OnPlayerEnter(Player&) {}
    virtual void OnPlayerStay(float, Player&) {}
    virtual void OnPlayerExit(Player&) {}

private:
    Type m_type;
    Box  m_box{};
    bool m_enabled = true;

    bool m_prevOverlap = false;
    bool m_triggered = false;
};

class LavaPool final : public Trap
{
public:
    LavaPool(const Box& box, int damagePerTick, float tickInterval);

protected:
    void OnPlayerStay(float dt, Player& player) override;

private:
    int   m_damagePerTick = 1;
    float m_tickInterval = 0.2f;
    float m_tickTimer = 0.f;
};

class PressurePlate final : public Trap
{
public:
    PressurePlate(const Box& box);
    void AddLinkedTrap(Trap* t);

protected:
    void OnPlayerEnter(Player& player) override;

private:
    std::vector<Trap*> m_linked;
};

class SpikePlate final : public Trap
{
public:
    SpikePlate(const Box& box, float upTime, float downTime, int damageOnHit, bool startDisabled);

    void Update(float dt, Player& player) override;

protected:
    void OnPlayerStay(float dt, Player& player) override;

private:
    float m_upTime = 1.f;
    float m_downTime = 1.f;
    int   m_damageOnHit = 10;

    bool  m_spikesUp = false;
    float m_phaseTimer = 0.f;

    float m_hitCooldown = 0.25f;
    float m_hitTimer = 0.f;
};

class TrapManager
{
public:
    template<typename T, typename... Args>
    T& Spawn(Args&&... args)
    {
        auto u = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *u;
        m_traps.emplace_back(std::move(u));
        return ref;
    }

    void Update(float dt, Player& player);
    void Render() const;

private:
    std::vector<std::unique_ptr<Trap>> m_traps;
};
