#include "UI.h"
#include "../Utils/ObjectPool.h"
#include <string>
#include "../Game/BuffCards.h"
#include "../Utils/AEExtras.h"
#include "Player/Player.h"
#include "../Utils/MeshGenerator.h"
#include "../Game/Time.h"
#include "../Game/Timer.h"
#include "../Game/GameOver.h"
#include <iostream>
#include "../Game/AudioManager.h"

namespace {
	std::string FormatTimeMMSSMS(double timeInSeconds) {
		int minutes = static_cast<int>(timeInSeconds) / 60;
		int seconds = static_cast<int>(timeInSeconds) % 60;
		int milliseconds = static_cast<int>((timeInSeconds - floor(timeInSeconds)) * 100); // two digits of ms

		// Format with leading zeros
		char buffer[16];
		sprintf_s(buffer, "%02d:%02d:%02d", minutes, seconds, milliseconds);
		return std::string(buffer);
	}
}

/*--------------------------------------------
			 General UI Functions
---------------------------------------------*/
void UI::Init(Player* _player) {
	damageTextFont = AEGfxCreateFont("Assets/m04.ttf", DAMAGE_TEXT_FONT_SIZE);
	gameOverFont = AEGfxCreateFont("Assets/Pixellari.ttf", GAME_OVER_TEXT_SIZE);
	healthVignette = AEGfxTextureLoad("Assets/Health_Vignette.png");
	healthVignetteMesh = MeshGenerator::GetRectMesh(1.0f, 1.0f);
	BuffCardManager::Init();
	BuffCardScreen::Init();
	UI::player = _player;
	InitCooldownMeshes();
	BuildEyelidMeshes();
}
void UI::Update() {
	BuffCardManager::Update();
	BuffCardScreen::Update();

	if (bossIntroActive)
	{
		UpdateBossIntro();
		return;
	}
	UpdateGameOverStatus();
	if (player->IsDead() && EyelidDone()) {
		UpdateGameOverButtonsAndText();
	}
}
void UI::Render() {
	DrawPlayerCooldownMeter();
	DrawHealthVignette();
	damageTextSpawner.Render();
	if (bossIntroActive)
	{
		RenderBossIntro();
		return;
	}
	BuffCardScreen::Render();
	if (player->IsDead()) {
		DrawEyelid();
		if (EyelidDone()) {
			DrawGameOverText();
		}
	}
}
void UI::Reset() {
	deadTimerAdded = false;
	ResetEyelid();
	gameOverTextFadeTimer = 0.0f;
	gameOverTextStage = 0;
}
void UI::Exit() {
	AEGfxDestroyFont(damageTextFont);
	AEGfxDestroyFont(gameOverFont);
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
	FreeEyelidMeshes();
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
		baseAlpha = 0.4f * t; // Strength of transparency.
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
	const int numViews = 24; // 20 discrete steps
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
		float percent = 1.0f - player->GetDashCooldownPercentage();
		
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
void UI::UpdateGameOverStatus() {
	if (!player->IsDead()) {
		deadTimerAdded = false;
		ResetEyelid();
		return;
	}
	if (deadTimerAdded && !TimerSystem::GetInstance().GetTimerByName("DeathAnim")) {
		deadTimerAdded = false;
	}
	if (!deadTimerAdded) {
		TimerSystem::GetInstance().AddTimer("DeathAnim", 2.5f, false);
		deadTimerAdded = true;
		ResetEyelid();
		
	}
	auto* timer = TimerSystem::GetInstance().GetTimerByName("DeathAnim");

	if (timer && timer->completed) {
		UpdateEyelid(static_cast<float>(Time::GetInstance().GetDeltaTime()));
		if (Time::GetInstance().GetTimeScale() > 0.0f) {
			Time::GetInstance().SetTimeScale(0);
		}
	}
}
void UI::UpdateGameOverButtonsAndText() {
	gameOverTextFadeTimer += static_cast<float>(Time::GetInstance().GetDeltaTime());
	if (gameOverTextFadeTimer >= 0.0f) gameOverTextStage = 1;
	if (gameOverTextFadeTimer >= 1.5f) gameOverTextStage = 2;
	if (gameOverTextFadeTimer >= 3.0f) gameOverTextStage = 3;
	float winW = static_cast<float>(AEGfxGetWindowWidth());
	float winH = static_cast<float>(AEGfxGetWindowHeight());

	// Approximate pixel width: font size * char count * scale * ~0.6
	float restartTextW = 11 * GAME_OVER_TEXT_SIZE * 1.0f * 0.6f; // "Restart Run" = 11 chars
	float menuTextW = 4 * GAME_OVER_TEXT_SIZE * 1.0f * 0.6f; // "Menu" = 4 chars

	// NDC left-edge of text pixel left-edge
	float restartLeftPx = ((RESTART_NDC_X + 1.0f) / 2.0f) * winW;
	float menuLeftPx = ((MENU_NDC_X + 1.0f) / 2.0f) * winW;

	// Center of hit box = left edge + half text width
	float restartCenterX = restartLeftPx + restartTextW * 0.5f;
	float menuCenterX = menuLeftPx + menuTextW * 0.5f;

	float restartY = ((1.0f - RESTART_NDC_Y) / 2.0f) * winH;
	float menuY = ((1.0f - MENU_NDC_Y) / 2.0f) * winH;

	AEVec2 btnSizeRestart = { restartTextW, 50.f };
	AEVec2 btnSizeMenu = { menuTextW,    50.f };

	bool hoverRestart = Button::CheckMouseInRectButton({ restartCenterX, restartY }, btnSizeRestart);
	bool hoverMenu = Button::CheckMouseInRectButton({ menuCenterX,    menuY }, btnSizeMenu);

	if (hoverRestart) {
		if (AEInputCheckTriggered(AEVK_LBUTTON)) {
			std::cout << "RESTART\n";
			Time::GetInstance().SetTimeScale(1.0f);
			restartRun = true;
			// restart
		}
	}
	if (hoverMenu) {
		if (AEInputCheckTriggered(AEVK_LBUTTON)) {
			std::cout << "MENU\n";
			// menu
		}
	}
}
void UI::DrawGameOverText() {
	float t = gameOverTextFadeTimer;

	// alpha for each string ?clamp 0 to 1, each starts 1.5s apart, takes 1s to fade in
	float a1 = AEClamp(t - 0.0f, 0.0f, 1.0f);
	float a2 = AEClamp(t - 1.5f, 0.0f, 1.0f);
	float a3 = AEClamp(t - 3.0f, 0.0f, 1.0f);

	if (a1 > 0.0f)
		AEGfxPrint(gameOverFont, "Fading...", -0.9f, 0.55f, 1.25f, 1.f, 1.f, 1.f, a1);
	if (a2 > 0.0f)
		AEGfxPrint(gameOverFont, "All is quiet.", -0.9f, 0.4f, 0.85f, 1.f, 1.f, 1.f, a2);
	if (a3 > 0.0f) {
		AEGfxPrint(gameOverFont, "Rest now.", -0.9f, 0.25f, 0.85f, 1.f, 1.f, 1.f, a3);
		f64 timeSpent = Time::GetInstance().GetScaledElapsedTime();
		std::string displayStr = "Moments spent - " + FormatTimeMMSSMS(timeSpent);
		AEGfxPrint(damageTextFont, displayStr.c_str(), -0.9f, 0.05f, 0.65f, 1.f, 1.f, 1.f, a3);

		float a4 = AEClamp(t - 4.0f, 0.0f, 1.0f); // buttons fade in last

		float winW = static_cast<float>(AEGfxGetWindowWidth());
		float winH = static_cast<float>(AEGfxGetWindowHeight());

		// Approximate pixel width: font size * char count * scale * ~0.6
		float restartTextW = 11 * GAME_OVER_TEXT_SIZE * 1.0f * 0.6f; // "Restart Run" = 11 chars
		float menuTextW = 4 * GAME_OVER_TEXT_SIZE * 1.0f * 0.6f; // "Menu" = 4 chars

		// NDC left-edge of text pixel left-edge
		float restartLeftPx = ((RESTART_NDC_X + 1.0f) / 2.0f) * winW;
		float menuLeftPx = ((MENU_NDC_X + 1.0f) / 2.0f) * winW;

		// Center of hit box = left edge + half text width
		float restartCenterX = restartLeftPx + restartTextW * 0.5f;
		float menuCenterX = menuLeftPx + menuTextW * 0.5f;

		float restartY = ((1.0f - RESTART_NDC_Y) / 2.0f) * winH;  // was missing / 2.0f
		float menuY = ((1.0f - MENU_NDC_Y) / 2.0f) * winH;

		AEVec2 btnSizeRestart = { restartTextW, 50.f };
		AEVec2 btnSizeMenu = { menuTextW,    50.f };

		bool hoverRestart = Button::CheckMouseInRectButton({ restartCenterX, restartY }, btnSizeRestart);
		bool hoverMenu = Button::CheckMouseInRectButton({ menuCenterX,    menuY }, btnSizeMenu);

		AEGfxPrint(gameOverFont, "Restart Run",
			RESTART_NDC_X, RESTART_NDC_Y, 1.0f,
			hoverRestart ? 1.f : 0.7f,
			hoverRestart ? 0.8f : 0.7f,
			hoverRestart ? 0.f : 0.7f,
			a4);

		AEGfxPrint(gameOverFont, "Menu",
			MENU_NDC_X, MENU_NDC_Y, 1.0f,
			hoverMenu ? 1.f : 0.7f,
			hoverMenu ? 0.8f : 0.7f,
			hoverMenu ? 0.f : 0.7f,
			a4);
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
		case DAMAGE_TYPE_HEAL:
			text.r = 0.1f, text.g = 1.0f, text.b = 0.25f;
			text.scale = 1.0f;
			break;
		case DAMAGE_TYPE_CRIT:
			text.r = 1.0f, text.g = 0.0f, text.b = 0.0f;
			text.scale = 1.15f;
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
		  Boss Intro functions
---------------------------------------*/
void UI::StartBossIntro()
{
	bossIntroActive = true;
	bossIntroPhase = BossIntroPhase::Black;
	bossIntroTimer = 0.0f;
	bossIntroTextAlpha = 0.0f;

	SetEyelidProgress(GetEyelidMaxProgress()); // start fully closed / black
}

bool UI::IsBossIntroActive()
{
	return bossIntroActive;
}

void UI::UpdateBossIntro()
{
	if (!bossIntroActive) return;

	float dt = static_cast<float>(Time::GetInstance().GetDeltaTime());
	bossIntroTimer += dt;

	switch (bossIntroPhase)
	{
	case BossIntroPhase::Black:
		if (bossIntroTimer >= 0.55f)
		{
			bossIntroPhase = BossIntroPhase::TextFadeIn;
			bossIntroTimer = 0.0f;
		}
		break;

	case BossIntroPhase::TextFadeIn:
		bossIntroTextAlpha = AEClamp(bossIntroTimer / 0.8f, 0.0f, 1.0f);
		if (bossIntroTimer >= 1.5f)
		{
			bossIntroPhase = BossIntroPhase::TextHold;
			bossIntroTimer = 0.0f;
			bossIntroTextAlpha = 1.0f;
		}
		break;

	case BossIntroPhase::TextHold:
		if (bossIntroTimer >= 1.0f)
		{
			bossIntroPhase = BossIntroPhase::EyelidOpen;
			bossIntroTimer = 0.0f;
		}
		break;

	case BossIntroPhase::EyelidOpen:
		UpdateEyelidOpen(dt);
		bossIntroTextAlpha = 1.0f - AEClamp(bossIntroTimer / 0.8f, 0.0f, 1.0f);

		if (EyelidFullyOpen())
		{
			bossIntroActive = false;
			bossIntroPhase = BossIntroPhase::None;
			bossIntroTimer = 0.0f;
			bossIntroTextAlpha = 0.0f;
		}
		break;

	default:
		break;
	}
}

void UI::RenderBossIntro()
{
	if (!bossIntroActive) return;

	DrawEyelid(); // since progress was set to fully closed initially

	if (bossIntroPhase == BossIntroPhase::TextFadeIn ||
		bossIntroPhase == BossIntroPhase::TextHold ||
		bossIntroPhase == BossIntroPhase::EyelidOpen)
	{
		// tune this starting x until the full sentence looks centered
		float x = -0.62f;
		float y = 0.05f;
		float scale = 0.9f;
		float alpha = bossIntroTextAlpha;

		AEGfxPrint(gameOverFont,
			"What an ",
			x, y, scale,
			1.0f, 1.0f, 1.0f, alpha);

		AEGfxPrint(gameOverFont,
			"ominous feeling....",
			x + 0.23f, y, scale,
			0.75f, 0.08f, 0.08f, alpha);
	}
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