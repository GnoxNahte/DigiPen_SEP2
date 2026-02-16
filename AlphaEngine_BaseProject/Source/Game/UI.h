#pragma once
#include <AEEngine.h>
#include <string>
#include "../Utils/ObjectPool.h"
#include "BuffCards.h"

// Enumeration types for the incoming damage type.
enum DAMAGE_TYPE {
	DAMAGE_TYPE_NORMAL,
	DAMAGE_TYPE_CRIT,
	DAMAGE_TYPE_RESIST,
	DAMAGE_TYPE_MISS,
	DAMAGE_TYPE_ENEMY_ATTACK,
	DAMAGE_TYPE_ENEMY_MISS
};


struct DamageText : public ObjectPoolItem {
	std::string damageType{}; // Type of damage to be printed. Crit, resist, normal etc.
	std::string damageNumber{}; // Numerical value of damage to be printed. Crit, resist, normal etc.
	f32 r{}, g{}, b{}; // RGB values of text.
	AEVec2 position{}; // Position of text.
	f32 alpha{}, scale{}, initialScale{}; // Alpha and scale values of text.
	f64 lifetime{}; // Lifetime of text, excluding neutral time.
	f64 maxLifetime{}; // Maximum lifetime of text to compute percentage of completion.
	f64 neutralTime{}; // Neutral time of the text before effects (scaling down and fading out).

	virtual void Init() override;
	virtual void OnGet() override;
	virtual void OnRelease() override;
	virtual void Exit() override;
	void Render();
};

class DamageTextSpawner {
public:
	void Update();
	void Render();

	DamageTextSpawner(int initialPoolSize);
	void SpawnDamageText(int damage, DAMAGE_TYPE type, AEVec2 position);
private:
	ObjectPool<DamageText> damageTextPool; // Object pool for damage text.
};

class UI
{
public:
	static void Init();
	static void Update();
	static void Render();
	static void Exit();
	inline static s8 GetDamageTextFont() { return damageTextFont; }
	inline static DamageTextSpawner& GetDamageTextSpawner() { return damageTextSpawner; }
	inline static const int GetMaxDamageTextInstances() { return MAX_DAMAGE_TEXT_INSTANCES; }
	inline static const int GetDamageTextFontSize() { return DAMAGE_TEXT_FONT_SIZE;  }

private:
	inline static s8 damageTextFont;
	static const int MAX_DAMAGE_TEXT_INSTANCES = 35;
	inline static DamageTextSpawner damageTextSpawner{ MAX_DAMAGE_TEXT_INSTANCES };
	static const int DAMAGE_TEXT_FONT_SIZE = 56;
};