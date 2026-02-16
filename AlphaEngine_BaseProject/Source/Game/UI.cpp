#include "UI.h"
#include "../Utils/ObjectPool.h"
#include "../Utils/MeshGenerator.h"
//#include "../Utils/AEExtras.h"
#include <string>

/*--------------------------------------------
			 General UI Functions
---------------------------------------------*/
void UI::Init() {
	damageTypeFont = AEGfxCreateFont("Assets/Bemock.ttf", DAMAGE_TYPE_FONT_SIZE);
	damageNumberFont = AEGfxCreateFont("Assets/Bemock.ttf", DAMAGE_NUMBER_FONT_SIZE);
	//UI::InitCards("Assets/0_CardBack.png");
}
void UI::Render() {
	damageTextSpawner.Render();
	//UI::DrawCards();
}
void UI::Exit() {
	AEGfxDestroyFont(damageTypeFont);
	AEGfxDestroyFont(damageNumberFont);
}
/*--------------------------------------
		  Damage Text Functions
---------------------------------------*/
void DamageText::Init() {}
void DamageText::OnGet() {
	lifetime = 3.0f; // 3 seconds
	scale = 1.f;
}
void DamageText::OnRelease() {}
void DamageText::Exit() {}
void DamageText::Render()
{
	s8 font = UI::GetDamageNumberFont();

	AEGfxPrint(font,
		damageNumber.c_str(),
		position.x,
		position.y,
		scale,
		r, g, b, alpha);
}
/*--------------------------------------
	  Damage Text Spawner Functions
---------------------------------------*/
DamageTextSpawner::DamageTextSpawner (int initialPoolSize) 
	: damageTextPool{ initialPoolSize } { /* empty by design */ }

void DamageTextSpawner::Update()
{
	std::cout << damageTextPool.GetSize() << '\n';
	for (int i = static_cast<int>(damageTextPool.GetSize()) - 1; i >= 0; --i)
	{
		DamageText& text = damageTextPool.pool[i];

		text.lifetime -= AEFrameRateControllerGetFrameTime();

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
	text.damageType = std::to_string(type);
	text.damageType = "CRIT!";
	text.position = position;
	text.r = 1.0f, text.g = 1.0f, text.b = 1.0f;
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

//void UI::PrintDamageText(int damage, AEVec2 position, f32 scale, f32 alpha, int damageCase) {
//	std::string damageType = {};
//	std::string damageNumber = {};
//	f32 r, g, b = {};
//	// TODO : centralize dmg text, and offset are temporarily hardcoded values to test functionality
//	switch (damageCase) {
//		case DAMAGE_TYPE_NORMAL:
//			r = 1.0f, g = 1.0f, b = 1.0f;
//			damageNumber = std::to_string(damage);
//			AEGfxPrint(damageNumberFont, damageNumber.c_str(), position.x + 0.04f, position.y - 0.1f, scale * 1.35f, r, g, b, alpha);
//			break;
//		case DAMAGE_TYPE_CRIT:
//			r = 1.0f, g = 0.0f, b = 0.0f;
//			damageType = "CRIT!";
//			AEGfxPrint(damageTypeFont, damageType.c_str(), position.x, position.y, scale * 1.25f * 1.5f, r, g, b, alpha);
//			damageNumber = std::to_string(damage);
//			AEGfxPrint(damageNumberFont, damageNumber.c_str(), position.x + 0.04f, position.y - 0.12f, scale * 1.5f, r, g, b, alpha);
//			break;
//		case DAMAGE_TYPE_RESIST:
//			r = 0.5f, g = 0.85f, b = 1.0f;
//			damageType = "RESIST!";
//			AEGfxPrint(damageTypeFont, damageType.c_str(), position.x, position.y, scale * 1.25f * 0.75f, r, g, b, alpha);
//			damageNumber = std::to_string(damage);
//			AEGfxPrint(damageNumberFont, damageNumber.c_str(), position.x + 0.04f, position.y - 0.1f, scale * 1.0f, r, g, b, alpha);
//			break;
//		case DAMAGE_TYPE_MISS:
//			r = 0.35f, g = 0.35f, b = 0.35f;
//			damageType = "MISS!";
//			AEGfxPrint(damageTypeFont, damageType.c_str(), position.x, position.y, scale * 1.25f * 0.75f, r, g, b, alpha + 0.5f);
//			break;
//		case DAMAGE_TYPE_ENEMY_ATTACK:
//			r = 1.0f, g = 0.2f, b = 0.85f;
//			damageNumber = std::to_string(damage);
//			AEGfxPrint(damageNumberFont, damageNumber.c_str(), position.x + 0.04f, position.y - 0.1f, scale * 1.35f, r, g, b, alpha);
//			break;
//		case DAMAGE_TYPE_ENEMY_MISS:
//			r = 0.8f, g = 0.35f, b = 0.65f;
//			damageType = "MISS!";
//			AEGfxPrint(damageTypeFont, damageType.c_str(), position.x, position.y, scale * 1.25f * 0.75f, r, g, b, alpha + 0.5f);
//			break;
//	}
//}