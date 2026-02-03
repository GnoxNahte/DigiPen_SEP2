#pragma once
#include <AEEngine.h>

// Enumeration types for the incoming damage type.
enum DAMAGE_TYPE{
	DAMAGE_TYPE_NORMAL,
	DAMAGE_TYPE_CRIT,
	DAMAGE_TYPE_RESIST,
	DAMAGE_TYPE_MISS,
	DAMAGE_TYPE_ENEMY_ATTACK,
	DAMAGE_TYPE_ENEMY_MISS
};

enum CARD_RARITY{
	RARITY_UNCOMMON,
	RARITY_RARE,
	RARITY_EPIC,
	RARITY_LEGENDARY
};

class UI
{
public:
	// Load fonts for damage. sizeType is for damage type text, sizeNumber is for damage number text.
	void static InitDamageFont(char const* filepath, int sizeType, int sizeNumber);

	// Print damage text at position with scale and alpha. damageCase references the enums defined above.
	void static PrintDamageText(int damage, AEVec2 position, f32 scale, f32 alpha, int damageCase);

	// Initialize cards
	void static InitCards(char const* filepath);

	// Draw cards.
	void static DrawCards();

	// Free card texture and mesh.
	void static FreeCards();

	void static Init();
	//void Update();
	void static Render();
	//void Exit();
	
	// Damage text variables
	//f32 alpha = 1.f;
	//f32 scale = 1.f;
	//int damageType = 0; // Testing of cycling through enum types.

private:
	inline static s8 damageTypeFont;
	inline static s8 damageNumberFont;
	inline static AEGfxTexture* cardTex;
	inline static AEGfxVertexList* cardMesh;

	// Card sprite dimensions
	static const int CARD_WIDTH = 750;
	static const int CARD_HEIGHT = 1050;
};