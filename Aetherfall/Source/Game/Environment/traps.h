/*!
@file  traps.h
@author  Jiang Chengyan
@brief  This file handles the traps of the game, including lava sprike and pressure plates.
It defines the behavior of these traps, how they interact with the player, and how they are rendered in the game world.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "AEEngine.h"
#include "../../Utils/Box.h"

class Player;

// -----------------------------------------------------------------------------
// Helper functions
// -----------------------------------------------------------------------------
bool IntersectsBox(const Box& a, const Box& b);
Box MakePlayerFeetBox(const Player& p);
Box MakePlayerBodyBox(const Player& p);

// -----------------------------------------------------------------------------
// Base Trap
// -----------------------------------------------------------------------------
class Trap
{
public:
    enum class Type
    {
        LavaPool,
        PressurePlate,
        SpikePlate
    };

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

    Type GetType() const { return m_type; }

    AEVec2 GetCenter() const;
    float GetDistanceToPoint(const AEVec2& point) const;

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

// -----------------------------------------------------------------------------
// LavaPool
// -----------------------------------------------------------------------------
class LavaPool final : public Trap
{
public:
    LavaPool(const Box& box, int damagePerTick, float tickInterval);

    void Render() const override;

protected:
    void OnPlayerEnter(Player& player) override;
    void OnPlayerStay(float dt, Player& player) override;

private:
    int   m_damagePerTick = 1;
    float m_tickInterval = 0.2f;
    float m_tickTimer = 0.f;
};

// -----------------------------------------------------------------------------
// PressurePlate
// -----------------------------------------------------------------------------
class PressurePlate final : public Trap
{
public:
    PressurePlate(const Box& box);

    void AddLinkedTrap(Trap* t);
    void Update(float dt, Player& player) override;
    void Render() const override;

    static void UnloadSharedRenderResources();

protected:
    void OnPlayerEnter(Player& player) override;

private:
    static AEGfxTexture* s_plateTexture;
    static AEGfxVertexList* s_plateMeshes[4];
    static bool             s_plateResourcesLoaded;

    static AEGfxVertexList* MakePlateMesh(int frame);
    static void             LoadSharedRenderResources();

    std::vector<Trap*> m_linked;

    int   m_animFrame = 0;
    float m_animTimer = 0.f;
};

// -----------------------------------------------------------------------------
// SpikePlate
// -----------------------------------------------------------------------------
class SpikePlate final : public Trap
{
public:
    SpikePlate(const Box& box,
        float      upTime,
        float      downTime,
        int        damageOnHit,
        bool       startDisabled);

    void Update(float dt, Player& player) override;
    void ActivateFromPlate(const Player& player);
    void OnPlayerEnter(Player& player) override;
    void Render() const override;

    static void LoadSharedRenderResources();
    static void UnloadSharedRenderResources();

protected:
    void OnPlayerStay(float dt, Player& player) override;

private:
    static AEGfxVertexList* MakeSpikeMesh(int frame);

    float m_upTime = 1.f;
    float m_downTime = 1.f;
    int   m_damageOnHit = 10;

    bool  m_spikesUp = false;
    float m_phaseTimer = 0.f;

    float m_hitCooldown = 0.5f;
    float m_hitTimer = 0.f;
    bool  m_lockedOn = false;

    int   m_animFrame = 0;   // 0~3
    float m_animTimer = 0.f;

    static AEGfxTexture* s_spikeTexture;
    static AEGfxVertexList* s_spikeMeshes[4];
    static bool             s_resourcesLoaded;
};

// -----------------------------------------------------------------------------
// TrapManager
// -----------------------------------------------------------------------------
class TrapManager
{
public:
    template <typename T, typename... Args>
    T& Spawn(Args&&... args)
    {
        auto u = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *u;
        m_traps.emplace_back(std::move(u));
        return ref;
    }

    void Update(float dt, Player& player);
    void Render() const;

    const Trap* GetClosestTrap(const AEVec2& point, bool enabledOnly = true) const;
    float GetClosestTrapDistance(const AEVec2& point, bool enabledOnly = true) const;
    float GetClosestLavaDistance(const AEVec2& point) const;

    static void UnloadAllSharedRenderResources();

private:
    std::vector<std::unique_ptr<Trap>> m_traps;
};