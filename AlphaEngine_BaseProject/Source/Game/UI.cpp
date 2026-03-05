#include "UI.h"
#include "../Utils/ObjectPool.h"
#include <string>
#include "BuffCards.h"
#include "../Utils/AEExtras.h"
#include "Player/Player.h"
#include "../Utils/MeshGenerator.h"
#include "../Game/Time.h"
#include "../Game/Timer.h"

/*--------------------------------------------
			 General UI Functions
---------------------------------------------*/
void UI::Init(Player* _player) {
	damageTextFont = AEGfxCreateFont("Assets/m04.ttf", DAMAGE_TEXT_FONT_SIZE);
	healthVignette = AEGfxTextureLoad("Assets/Health_Vignette.png");
	healthVignetteMesh = MeshGenerator::GetRectMesh(1.0f, 1.0f);
	//cooldownTimerMesh = MeshGenerator::GetCircleMesh(3);
	BuffCardManager::Init();
	BuffCardScreen::Init();
	UI::player = _player;
	InitCooldownMeshes();
}
void UI::Update() {
	BuffCardManager::Update();
	BuffCardScreen::Update();
}
void UI::Render() {
	DrawPlayerCooldownMeter();
	DrawHealthVignette();
	damageTextSpawner.Render();
	BuffCardScreen::Render();
}
void UI::Exit() {
	AEGfxDestroyFont(damageTextFont);
	if (healthVignetteMesh) {
		AEGfxMeshFree(healthVignetteMesh);
	}
	if (healthVignette) {
		AEGfxTextureUnload(healthVignette);
	}
	for (AEGfxVertexList*& mesh : cooldownMeshes) {
		AEGfxMeshFree(mesh);
		mesh = nullptr;
	}
	BuffCardScreen::Exit();
	UI::player = nullptr;
}
void UI::DrawHealthVignette() {
	// --- Rotation ---
	AEMtx33 rotate{ 0 };
	AEMtx33Identity(&rotate);

	// --- Scale ---
	AEMtx33 scale;
	AEMtx33Scale(&scale,
		static_cast<f32>(AEGfxGetWindowWidth() * 1.05f),
		static_cast<f32>(AEGfxGetWindowHeight() * 1.05f));

	// --- Translate ---
	AEMtx33 translate;
	AEMtx33Trans(&translate,
		Camera::position.x * Camera::scale,
		Camera::position.y * Camera::scale);

	// --- Combine ---
	AEMtx33 transform;
	AEMtx33Concat(&transform, &rotate, &scale);
	AEMtx33Concat(&transform, &translate, &transform);

	// --- Health values ---
	int playerHealth = player->GetHealth();
	int maxHealth = player->GetStats().maxHealth;
	float healthFraction = static_cast<float>(playerHealth) / static_cast<float>(maxHealth);

	float baseAlpha = 0.0f;

	// Only activate below 50%
	if (healthFraction < 0.5f)
	{
		float t = (0.5f - healthFraction) / 0.25f;
		t = AEClamp(t, 0.0f, 1.0f);
		baseAlpha = 0.75f * t; // Strength of transparency.
	}

	// --- Heartbeat pulse ---
	float pulse = 0.0f;

	if (healthFraction <= 0.5f)
	{
		float totalTime = static_cast<f32>(Time::GetInstance().GetScaledElapsedTime());

		float beatSpeed = 0.9f;   // Increase for faster heartbeat
		float beat = fmod(totalTime * beatSpeed, 1.0f);

		if (beat < 0.15f) // Rise duration
		{
			pulse = (beat / 0.15f);           // Fast rise
		}
		else if (beat < 0.35f) // Fall duration
		{
			pulse = 1.0f - ((beat - 0.15f) / 0.2f); // Slow fall
		}
		else
		{
			pulse = 0.0f; // Rest period
		}

		pulse *= 0.2f; // pulse strength
	}

	float finalAlpha = AEClamp(baseAlpha + pulse, 0.0f, 1.0f);

	// --- Apply ---
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetTransparency(finalAlpha);
	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
	AEGfxTextureSet(healthVignette, 0, 0);
	AEGfxSetTransform(transform.m);
	AEGfxMeshDraw(healthVignetteMesh, AE_GFX_MDM_TRIANGLES);
}
void UI::InitCooldownMeshes() {
	const int numViews = 20; // 20 discrete steps
	cooldownMeshes.resize(numViews + 1); // 0% -> 100%

	for (int i = 0; i <= numViews; ++i)
	{
		float percent = i / static_cast<float>(numViews); // 0.0 -> 1.0
		cooldownMeshes[i] = MeshGenerator::GetCooldownMesh(
			20.0f,       // radius
			0xFFFFFFFF,  // white
			60,          // smoothness
			percent      // fill amount
		);
	}
}
void UI::DrawPlayerCooldownMeter() {
	{
		if (AEInputCheckCurr(AEVK_Z)) {
			TimerSystem::GetInstance().AddTimer("Player Cooldown Timer", player->GetStats().dashCooldown);
		}
		auto* timer = TimerSystem::GetInstance().GetTimerByName("Player Cooldown Timer");
		if (!timer) return;

		float percent = AEClamp(1.0f - static_cast<float>(timer->percentage), 0.0f, 1.0f);

		// Pick the closest precomputed mesh
		int numViews = static_cast<int>(cooldownMeshes.size()) - 1;
		int meshIndex = static_cast<int>(percent * numViews + 0.5f); // round to nearest
		meshIndex = std::clamp(meshIndex, 0, numViews);

		AEGfxVertexList* meshToDraw = cooldownMeshes[meshIndex];
		if (!meshToDraw) return;

		// Calculate screen position
		float Xoffset = -0.1f * Camera::scale, Yoffset = 1.f * Camera::scale;
		float screenX = player->GetPosition().x * Camera::scale + Camera::position.x + Xoffset;
		float screenY = player->GetPosition().y * Camera::scale + Camera::position.y + Yoffset;

		// Apply transform
		AEMtx33 transform;
		AEMtx33Identity(&transform);
		AEMtx33Trans(&transform, screenX, screenY);

		AEGfxSetRenderMode(AE_GFX_RM_COLOR);
		AEGfxSetBlendMode(AE_GFX_BM_BLEND);
		AEGfxSetTransform(transform.m);

		// Draw the precomputed mesh
		AEGfxMeshDraw(meshToDraw, AE_GFX_MDM_TRIANGLES);
		AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
	}
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