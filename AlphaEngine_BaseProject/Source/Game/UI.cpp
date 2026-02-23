#include "UI.h"
#include "../Utils/ObjectPool.h"
#include "../Utils/MeshGenerator.h"
#include <string>
#include "BuffCards.h"
#include "../Utils/AEExtras.h"
#include "Camera.h"

/*--------------------------------------------
			 General UI Functions
---------------------------------------------*/
void UI::Init() {
	damageTextFont = AEGfxCreateFont("Assets/m04.ttf", DAMAGE_TEXT_FONT_SIZE);
	BuffCardManager::Init();
	BuffCardScreen::Init();
}
void UI::Update() {
	BuffCardManager::Update();
	BuffCardScreen::Update();
}
void UI::Render() {
	damageTextSpawner.Render();
	BuffCardScreen::Render();
}
void UI::Exit() {
	AEGfxDestroyFont(damageTextFont);
	BuffCardScreen::Exit();
}
/*--------------------------------------
		  Damage Text Functions
---------------------------------------*/
void DamageText::Init() {}
void DamageText::OnGet() {
	neutralTime = 0.35f; // Neutral state of damage numbers before effects.
	lifetime = 0.75f; // Lifetime of text with effects.
	maxLifetime = lifetime; // Maximum lifetime for percentage computation.
}
void DamageText::OnRelease() {}
void DamageText::Exit() {}
void DamageText::Render()
{
	// Get window dimensions
	f32 windowWidth = static_cast<f32>(AEGfxGetWindowWidth());
	f32 windowHeight = static_cast<f32>(AEGfxGetWindowHeight());

	// Calculate text width in pixels (approximate)
	// Each character is roughly fontsize * 0.6 pixels wide
	f32 numberPixelWidth = damageNumber.length() * UI::GetDamageTextFontSize() * 0.6f;
	f32 typePixelWidth = damageType.length() * UI::GetDamageTextFontSize() * 0.6f;

	// Convert pixel offset to normalized coordinates [-1, 1]
	// Divide by window width and multiply by 2 (since range is -1 to 1, total span of 2)
	f32 numberOffsetX = (numberPixelWidth / windowWidth) * scale;
	f32 typeOffsetX = (typePixelWidth / windowWidth) * scale;

	// Vertical spacing in normalized coordinates
	f32 verticalSpacing = (UI::GetDamageTextFontSize() / windowHeight) * 2.f * scale;

	s8 font = UI::GetDamageTextFont();
	AEVec2 viewportPos;
	AEExtras::WorldToViewportPosition(position, viewportPos);
	viewportPos.x = viewportPos.x * 2 - 1.f;
	viewportPos.y = viewportPos.y * 2 - 1.f;

	// Print Damage Type.
	AEGfxPrint(font,
		damageType.c_str(),
		viewportPos.x - typeOffsetX * 0.5f,
		viewportPos.y + verticalSpacing * 0.5f,
		scale,
		r, g, b, alpha);
	// Print Damage Number.
	AEGfxPrint(font,
		damageNumber.c_str(),
		viewportPos.x - numberOffsetX * 0.5f,
		viewportPos.y - verticalSpacing * 0.5f,
		scale,
		r, g, b, alpha);
}
/*--------------------------------------
	  Damage Text Spawner Functions
---------------------------------------*/
DamageTextSpawner::DamageTextSpawner (int initialPoolSize) // Constructor
	: damageTextPool{ initialPoolSize } { /* empty by design */ }

void DamageTextSpawner::Update()
{
	for (int i = static_cast<int>(damageTextPool.GetSize()) - 1; i >= 0; --i)
	{
		DamageText& text = damageTextPool.pool[i];
		text.neutralTime -= AEFrameRateControllerGetFrameTime();
		if (text.neutralTime <= 0.f) {
			text.lifetime -= AEFrameRateControllerGetFrameTime();
			f32 lifeRatio = static_cast<f32>(text.lifetime / text.maxLifetime);
			text.alpha = lifeRatio;
			text.scale = text.initialScale * lifeRatio;
		}
		if (text.lifetime <= 0.f)
		{
			damageTextPool.Release(text);
		}
	}
}
void DamageTextSpawner::Render() {
	for (size_t i = 0; i < damageTextPool.GetSize(); ++i)
	{
		damageTextPool.pool[i].Render();
	}
}
void DamageTextSpawner::SpawnDamageText(int damage, DAMAGE_TYPE type, AEVec2 position) {
	if (damageTextPool.GetSize() > UI::GetMaxDamageTextInstances()) {
		return;
	}
	DamageText& text = damageTextPool.Get();
	text.damageNumber = std::to_string(damage);
	text.damageType = "";
	// Account for damage type and change their colors accordingly.
	switch (type) {
		case DAMAGE_TYPE_NORMAL:
			text.r = 1.0f, text.g = 1.0f, text.b = 1.0f;
			text.scale = 1.0f;
			break;
		case DAMAGE_TYPE_CRIT:
			text.r = 1.0f, text.g = 0.0f, text.b = 0.0f;
			text.scale = 1.25f;
			text.damageType = "CRT!";
			break;
		case DAMAGE_TYPE_RESIST:
			text.r = 0.5f, text.g = 0.85f, text.b = 1.0f;
			text.scale = 0.75f;
			text.damageType = "RES!";
			break;
		case DAMAGE_TYPE_MISS:
			text.r = 0.85f, text.g = 0.85f, text.b = 0.85f;
			text.scale = 0.75f;
			text.damageType = "MISS!";
			text.damageNumber = "";
			break;
		case DAMAGE_TYPE_ENEMY_ATTACK:
			text.r = 1.0f, text.g = 0.2f, text.b = 0.85f;
			text.scale = 1.0f;
			break;
		case DAMAGE_TYPE_ENEMY_MISS:
			text.r = 0.8f, text.g = 0.35f, text.b = 0.65f;
			text.scale = 0.75f;
			text.damageType = "MISS!";
			text.damageNumber = "";
			break;
	}
	text.initialScale = text.scale;
	text.position = position;
	text.alpha = 1.0f;
	text.OnGet();
}
/*--------------------------------------
			Button Functions
---------------------------------------*/
bool Button::CheckMouseInRectButton(AEVec2 pos, AEVec2 size) {
	s32 mouseX, mouseY;
	AEInputGetCursorPosition(&mouseX, &mouseY);
	return (mouseX >= pos.x - size.x * 0.5f &&
		mouseX <= pos.x + size.x * 0.5f &&
		mouseY >= pos.y - size.y * 0.5f &&
		mouseY <= pos.y + size.y * 0.5f);
}
