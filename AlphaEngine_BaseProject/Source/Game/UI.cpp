#include "UI.h"
#include "../Utils/ObjectPool.h"
#include "../Utils/MeshGenerator.h"
//#include "../Utils/AEExtras.h"
#include <string>

/*--------------------------------------------
			 General UI Functions
---------------------------------------------*/
void UI::Init() {
	damageTextFont = AEGfxCreateFont("Assets/Bemock.ttf", DAMAGE_TEXT_FONT_SIZE);
	//UI::InitCards("Assets/0_CardBack.png");
}
void UI::Render() {
	damageTextSpawner.Render();
	//UI::DrawCards();
}
void UI::Exit() {
	AEGfxDestroyFont(damageTextFont);
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

	// Print Damage Type.
	AEGfxPrint(font,
		damageType.c_str(),
		position.x - typeOffsetX * 0.5f,
		position.y + verticalSpacing * 0.5f,
		scale,
		r, g, b, alpha);
	// Print Damage Number.
	AEGfxPrint(font,
		damageNumber.c_str(),
		position.x - numberOffsetX * 0.5f,
		position.y - verticalSpacing * 0.5f,
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
	//std::cout << damageTextPool.GetSize() << '\n'; // debug
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
			text.damageType = "CRIT!";
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

//void UI::InitCards(char const* filepath) {
//	// Initialize card mesh and textures.
//	cardMesh = MeshGenerator::GetRectMesh(1,1);
//	cardTex = AEGfxTextureLoad(filepath);
//}
//void UI::DrawCards() {
//	// Create a scale matrix that scales by scaling factor.
//	AEMtx33 scale = { 0 };
//	float scalingFactor = 0.5f;
//	AEMtx33Scale(&scale, CARD_WIDTH * scalingFactor, CARD_HEIGHT * scalingFactor);
//
//	// Create a rotation matrix, base card doesn't rotate.
//	AEMtx33 rotate = { 0 };
//	AEMtx33Rot(&rotate, 0);
//
//	// Create a translation matrix that translates by
//	// 200 in the x-axis and 100 in the y-axis
//	AEMtx33 translate = { 0 };
//	AEMtx33Trans(&translate, (AEGfxGetWinMaxX()/2), (AEGfxGetWinMaxY()/2));
//
//	// Concatenate the matrices into the 'transform' variable.
//	AEMtx33 transform = { 0 };
//	AEMtx33Concat(&transform, &rotate, &scale);
//	AEMtx33Concat(&transform, &translate, &transform);
//
//	// Draw something with texture.
//	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
//
//	// Set the the color to multiply to white, so that the sprite can 
//	// display the full range of colors (default is black).
//	AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
//
//	// Set the color to add to nothing, so that we don't alter the sprite's color
//	AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
//
//	// Set blend mode to AE_GFX_BM_BLEND, which will allow transparency.
//	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
//	AEGfxSetTransparency(1.0f);
//
//	// Tell Alpha Engine to use the texture stored in pTex
//	AEGfxTextureSet(cardTex, 0, 0);
//
//	// Tell Alpha Engine to use the matrix in 'transform' to apply onto all 
//	// the vertices of the mesh that we are about to choose to draw in the next line.
//	AEGfxSetTransform(transform.m);
//
//	// Tell Alpha Engine to draw the mesh with the above settings.
//	AEGfxMeshDraw(cardMesh, AE_GFX_MDM_TRIANGLES);
//}

//void UI::FreeCards() {
//	// Free card textures and meshes.
//	AEGfxTextureUnload(cardTex);
//	AEGfxMeshFree(cardMesh);
//}
