#pragma once
#include <AEEngine.h>
#include "Timer.h"

//struct DamageTextInfo
//{
//	int damage = 0;
//	AEVec2 position;
//	f64 scale = 0.0f, critScale = 0.0f, r = 1.0f, g = 1.0f, b = 1.0f, alpha = 1.0f;
//
//};

enum {
	DAMAGE_TYPE_NORMAL,
	DAMAGE_TYPE_CRIT,
	DAMAGE_TYPE_RESIST
};

class UI
{
public:
	void static InitDamageFont(char const* filepath, int sizeType, int sizeNumber);
	void static PrintDamageText(int damage, AEVec2 position, f32 scale, f32 alpha, int damageCase);

private:
	inline static s8 damageTypeFont;
	inline static s8 damageNumberFont;
	inline static TimerSystem timerSystem;
};