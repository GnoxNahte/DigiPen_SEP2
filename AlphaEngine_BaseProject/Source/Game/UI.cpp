#include "UI.h"
#include <string>

void UI::InitDamageFont(char const* filepath, int sizeType, int sizeNumber) {
	damageTypeFont = AEGfxCreateFont(filepath, sizeType);
	damageNumberFont = AEGfxCreateFont(filepath, sizeNumber);
}

void UI::PrintDamageText(int damage, AEVec2 position, f32 scale, f32 alpha, int damageCase) {
	std::string damageType = {};
	std::string damageNumber = {};
	f32 r, g, b = {};
	switch (damageCase) {
		case DAMAGE_TYPE_NORMAL:
			r = 1.0f, g = 1.0f, b = 1.0f;
			damageNumber = std::to_string(damage);
			AEGfxPrint(damageNumberFont, damageNumber.c_str(), position.x + 0.04f, position.y - 0.1f, scale * 1.35f, r, g, b, alpha);
			break;
		case DAMAGE_TYPE_CRIT:
			r = 1.0f, g = 0.0f, b = 0.0f;
			damageType = "CRIT!";
			AEGfxPrint(damageTypeFont, damageType.c_str(), position.x, position.y, scale * 1.25f * 1.5f, r, g, b, alpha);
			damageNumber = std::to_string(damage);
			AEGfxPrint(damageNumberFont, damageNumber.c_str(), position.x + 0.04f, position.y - 0.12f, scale * 1.5f, r, g, b, alpha);
			break;
		case DAMAGE_TYPE_RESIST:
			r = 0.5f, g = 0.85f, b = 1.0f;
			damageType = "RESIST!";
			AEGfxPrint(damageTypeFont, damageType.c_str(), position.x, position.y, scale * 1.25f * 0.75f, r, g, b, alpha);
			damageNumber = std::to_string(damage);
			AEGfxPrint(damageNumberFont, damageNumber.c_str(), position.x + 0.04f, position.y - 0.1f, scale * 1.0f, r, g, b, alpha);
			break;
		case DAMAGE_TYPE_MISS:
			r = 0.35f, g = 0.35f, b = 0.35f;
			damageType = "MISS!";
			AEGfxPrint(damageTypeFont, damageType.c_str(), position.x, position.y, scale * 1.25f * 0.75f, r, g, b, alpha + 0.5f);
			break;
		case DAMAGE_TYPE_ENEMY_ATTACK:
			r = 1.0f, g = 0.2f, b = 0.85f;
			damageNumber = std::to_string(damage);
			AEGfxPrint(damageNumberFont, damageNumber.c_str(), position.x + 0.04f, position.y - 0.1f, scale * 1.35f, r, g, b, alpha);
			break;
		case DAMAGE_TYPE_ENEMY_MISS:
			r = 0.8f, g = 0.35f, b = 0.65f;
			damageType = "MISS!";
			AEGfxPrint(damageTypeFont, damageType.c_str(), position.x, position.y, scale * 1.25f * 0.75f, r, g, b, alpha + 0.5f);
			break;
	}
}