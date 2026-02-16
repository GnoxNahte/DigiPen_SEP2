#pragma once
#include <AEEngine.h>
#include <string>
#include "../Utils/ObjectPool.h"

// Enumeration types for the incoming damage type.
enum DAMAGE_TYPE {
	DAMAGE_TYPE_NORMAL,
	DAMAGE_TYPE_CRIT,
	DAMAGE_TYPE_RESIST,
	DAMAGE_TYPE_MISS,
	DAMAGE_TYPE_ENEMY_ATTACK,
	DAMAGE_TYPE_ENEMY_MISS
};

enum CARD_RARITY {
	RARITY_UNCOMMON,
	RARITY_RARE,
	RARITY_EPIC,
	RARITY_LEGENDARY
};


struct DamageText : public ObjectPoolItem {
	std::string damageType{}; // Type of damage to be printed. Crit, resist, normal etc.
	std::string damageNumber{}; // Numerical value of damage to be printed. Crit, resist, normal etc.
	f32 r{}, g{}, b{}; // RGB values of text.
	AEVec2 position{}; // Position of text.
	f32 alpha{}, scale{}; // Alpha and scale values of text.
	f64 lifetime{}; // Lifetime of text.

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
	static void Render();
	static void Exit();
	inline static s8 GetDamageTypeFont() { return damageTypeFont; }
	inline static s8 GetDamageNumberFont() { return damageNumberFont; }
	inline static DamageTextSpawner& GetDamageTextSpawner() { return damageTextSpawner; }
	inline static const int GetMaxDamageTextInstances() { return MAX_DAMAGE_TEXT_INSTANCES; }
	// Print damage text at position with scale and alpha. damageCase references the enums defined above.
	//void static PrintDamageText(int damage, AEVec2 position, f32 scale, f32 alpha, int damageCase);

	//// Initialize cards
	//void static InitCards(char const* filepath);

	//// Draw cards.
	////void static DrawCards();

	//// Free card texture and mesh.
	//void static FreeCards();

	
	// Damage text variables
	//f32 alpha = 1.f;
	//f32 scale = 1.f;
	//int damageType = 0; // Testing of cycling through enum types.

private:
	inline static s8 damageTypeFont;
	inline static s8 damageNumberFont;
	static const int MAX_DAMAGE_TEXT_INSTANCES = 20;
	inline static DamageTextSpawner damageTextSpawner{ MAX_DAMAGE_TEXT_INSTANCES };
	static const int DAMAGE_TYPE_FONT_SIZE = 48;
	static const int DAMAGE_NUMBER_FONT_SIZE = 52;

//	inline static AEGfxTexture* cardTex;
//	inline static AEGfxVertexList* cardMesh;
//	// Card sprite dimensions
//	static const int CARD_WIDTH = 750;
//	static const int CARD_HEIGHT = 1050;
};