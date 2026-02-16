//#include <iostream>
#include "BuffCards.h"
#include "Camera.h"
#include "../Utils/MeshGenerator.h"
#include "../Utils/AEExtras.h"
#include "../Game/UI.h"
#include "Timer.h"

BuffCard::BuffCard(CARD_RARITY cr, // Constructor
	CARD_TYPE ct,
	std::string cName,
	std::string cDesc,
	AEVec2 pos) :
	rarity{ cr }, type{ ct }, cardName{ cName }, cardDesc{ cDesc }, cardPos{ pos } {/* empty by design */
}

void BuffCardScreen::Init() {
	rectMesh = MeshGenerator::GetRectMesh(1.0f, 1.0f);
	cardMesh = MeshGenerator::GetRectMesh(1.0f, 1.0f);
	cardBackTex = AEGfxTextureLoad("Assets/0_CardBack.png");
	buffPromptFont = AEGfxCreateFont("Assets/m04.ttf", BUFF_PROMPT_FONT_SIZE);
}
void BuffCardScreen::RandomizeCards() {

}
// Draw a black overlay when drawing cards.
void BuffCardScreen::DrawBlackOverlay() {
	AEMtx33 scale, rotate, translate, transform;
	// Intentionally scale the overlay to be big so that it covers the entire screen.
	AEMtx33Scale(&scale, static_cast<f32>(AEGfxGetWindowWidth() * 2), 
		static_cast<f32>(AEGfxGetWindowHeight() * 2));
	AEMtx33Rot(&rotate, 0.0f);
	AEMtx33Trans(&translate, Camera::position.x * Camera::scale, Camera::position.y * Camera::scale);

	AEMtx33Concat(&transform, &rotate, &scale);
	AEMtx33Concat(&transform, &translate, &transform);

	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	AEGfxSetColorToMultiply(0.f, 0.f, 0.f, 0.f);
	AEGfxSetColorToAdd(0.f,0.f,0.f,0.85f);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetTransparency(0.85f);

	AEGfxSetTransform(transform.m);
	AEGfxMeshDraw(rectMesh, AE_GFX_MDM_TRIANGLES);
}
void BuffCardScreen::DrawPromptText() {
	if (!textLoading) {
		TimerSystem::GetInstance().AddTimer("Choose Buff Timer", 1.2f);
		textLoading = true;
	}
	// Approximate text width calculation
	std::string text = "Choose a buff.";
	f32 charWidth = 0.044f;  // Approximate width per character in normalized coords
	f32 textWidth = text.length() * charWidth;
	f32 centeredX = -textWidth * 0.5f;  // Offset by half width to center
	f32 textVert = 0.68f;
	if (TimerSystem::GetInstance().GetTimerByName("Choose Buff Timer") &&
		!TimerSystem::GetInstance().GetTimerByName("Choose Buff Timer")->completed) { // Fade in.
		AEGfxPrint(buffPromptFont,
			text.c_str(),
			centeredX,
			textVert,
			1.0f,
			1.0f, 1.0f, 1.0f,
			static_cast<f32>(TimerSystem::GetInstance().GetTimerByName("Choose Buff Timer")->percentage));
	}
	else {
		// Stay at 1 alpha.
		AEGfxPrint(buffPromptFont,
			text.c_str(),
			centeredX,
			textVert,
			1.0f,
			1.0f, 1.0f, 1.0f,
			1.0f);
	}
}
// Draw buff cards.
void BuffCardScreen::DrawDeck() {
	// Scaling matrix.
	AEMtx33 scale = { 0 };
	const f32 CARD_SIZE_MODIFIER = 0.42f;
	AEMtx33Scale(&scale, CARD_WIDTH * CARD_SIZE_MODIFIER, CARD_HEIGHT * CARD_SIZE_MODIFIER);

	// Rotation matrix.
	AEMtx33 rotate = { 0 };
	AEMtx33Rot(&rotate, 0);

	// Set render state
	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
	AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
	AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetTransparency(1.0f);

	// Set texture
	AEGfxTextureSet(cardBackTex, 0, 0);

	// Draw 3 cards with spacing
	const int NUM_CARDS = 3;
	const f32 CARD_SPACING = 450.f; // Adjust spacing between cards

	for (int i = 0; i < NUM_CARDS; ++i) {
		// Calculate offset for this card
		// Center the group: offset = (i - 1) * spacing
		// This makes card 0 at -spacing, card 1 at 0, card 2 at +spacing
		f32 offsetX = (i - 1) * CARD_SPACING;

		// Translation matrix (different for each card)
		AEMtx33 translate = { 0 };
		AEMtx33Trans(&translate,
			Camera::position.x * Camera::scale + offsetX,
			Camera::position.y * Camera::scale);

		// Concatenate matrices
		AEMtx33 transform = { 0 };
		AEMtx33Concat(&transform, &rotate, &scale);
		AEMtx33Concat(&transform, &translate, &transform);

		// Draw this card
		AEGfxSetTransform(transform.m);
		AEGfxMeshDraw(cardMesh, AE_GFX_MDM_TRIANGLES);
	}
}
/*---------------------------------------

TODO: this function should shuffle out 3 cards from the deck (middle), accounting for shuffle buff triggering this function.

shuffle out from deck (middle).
align cards to their respective position as they move. (one by one)
scale to simulate flipping of cards.

rarity emission after they appear.

default first selected to left, scaling up in size to show selected one.

-----------------------------------------*/
//void BuffCardScreen::DrawBuffCards() {
//	// Scaling matrix.
//	AEMtx33 scale = { 0 };
//	const f32 CARD_SIZE_MODIFIER = 0.25f;
//	AEMtx33Scale(&scale, CARD_WIDTH * CARD_SIZE_MODIFIER, CARD_HEIGHT * CARD_SIZE_MODIFIER);
//
//	// Rotation matrix.
//	AEMtx33 rotate = { 0 };
//	AEMtx33Rot(&rotate, 0);
//
//	// Translation matrix.
//	AEMtx33 translate = { 0 };
//	AEMtx33Trans(&translate, Camera::position.x * Camera::scale,
//		Camera::position.y * Camera::scale - 185.f);
//
//	// Concatenate the matrices into the 'transform' variable.
//	AEMtx33 transform = { 0 };
//	AEMtx33Concat(&transform, &rotate, &scale);
//	AEMtx33Concat(&transform, &translate, &transform);
//
//	// Set render state
//	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
//	AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
//	AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
//	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
//	AEGfxSetTransparency(1.0f);
//
//	// Draw
//	AEGfxTextureSet(cardBackTex, 0, 0);
//	AEGfxSetTransform(transform.m);
//	AEGfxMeshDraw(cardMesh, AE_GFX_MDM_TRIANGLES);
//}
/*----------------------------

TODO: this function simulates discarding all cards for a shuffle buff, which a shuffle buff would call this function
then call drawbuffcards again after all cards have been cleared from screen.

---------------------------------*/
void BuffCardScreen::DiscardCards() {

}
void BuffCardScreen::Render() {
	DrawBlackOverlay();
	DrawPromptText();
	//DrawBuffCards();
	DrawDeck();
	//if (AEInputCheckTriggered(AEVK_P)) { // Remember to set textloading back to false for fadeonce flag.
	//	textLoading = false;
	//}
}
void BuffCardScreen::Exit() {
	if (cardMesh) {
		// Free card meshes.
		AEGfxMeshFree(cardMesh);
	}
	if (cardBackTex) {
		// Free card textures.
		AEGfxTextureUnload(cardBackTex);
	}
}