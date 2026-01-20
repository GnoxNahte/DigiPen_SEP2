#pragma once
#include <AEEngine.h>

// Enumeration types for the incoming damage type.
enum {
	DAMAGE_TYPE_NORMAL,
	DAMAGE_TYPE_CRIT,
	DAMAGE_TYPE_RESIST,
	DAMAGE_TYPE_ENEMY_ATTACK,
	DAMAGE_TYPE_MISS
};

class UI
{
public:
	// Load fonts for damage. sizeType is for damage type text, sizeNumber is for damage number text.
	void static InitDamageFont(char const* filepath, int sizeType, int sizeNumber);

	// Print damage text at position with scale and alpha. damageCase references the enums defined above.
	void static PrintDamageText(int damage, AEVec2 position, f32 scale, f32 alpha, int damageCase);

private:
	inline static s8 damageTypeFont;
	inline static s8 damageNumberFont;
};