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
	inline static const int GetDamageTextFontSize() { return DAMAGE_TEXT_FONT_SIZE; }

private:
	static const int MAX_DAMAGE_TEXT_INSTANCES = 35;
	static const int DAMAGE_TEXT_FONT_SIZE = 56;
	inline static s8 damageTextFont;
	inline static DamageTextSpawner damageTextSpawner{ MAX_DAMAGE_TEXT_INSTANCES };
};
// Enums for button states, to determine how the button should react to player interaction and what visuals to show.
enum BUTTON_STATE {
	BUTTON_NEUTRAL, // The default button state without interaction / exit.
	BUTTON_HOVERONCE, // The button state when player first hovers over a button.
	BUTTON_HOVERHELD, // The button state when player stays hovered over a button.
	BUTTON_CLICKED, // The button state when player releases while mouse is in button. (cursor up)
	BUTTON_RELEASED, // The button state when player clicks down (cursor down).
	BUTTON_DISABLED // The button state when disabled, as a separate handling case.
};
class Button {
public:
	Button() : size{ 0,0 }, pos{ 0,0 }, hoverOnce{ false }, buttonState{ BUTTON_NEUTRAL } { /* empty by design */ }
	Button(AEVec2 size, AEVec2 pos) : size{ size }, pos{ pos }, hoverOnce{ false }, buttonState{ BUTTON_NEUTRAL } { /* empty by design */ }
	static bool CheckMouseInRectButton(AEVec2 pos, AEVec2 size);

private:
	AEVec2 size; // Size of the button (x is width, y is height). Expressed in percentage of window width and height (0-1).
	AEVec2 pos; // Position of the button (x-coordinates, y-coordinates). Expressed in percentage of window width and height (0-1).
	bool hoverOnce; // Checks whether the button is hovered once. Used to handle on first hover cases, e.g. sound.
	BUTTON_STATE buttonState; // Enum of button's state.

	// Colors to be KIV as currently the buttons are invisible.
	// //CP_Color defaultColor; // Default color of the button.
	// //CP_Color hoverColor; // Hover color of the button.
	// //CP_Color clickedColor; // Clicked color of the button.
	// //CP_Color drawColor; // Initial Draw color of the button.
};