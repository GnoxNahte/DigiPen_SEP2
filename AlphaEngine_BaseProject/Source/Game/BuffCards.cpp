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
	cardFrontTex[0] = AEGfxTextureLoad("Assets/Hermes_Favor.png");
	cardFrontTex[1] = AEGfxTextureLoad("Assets/Iron_Defence.png");
	cardFrontTex[2] = AEGfxTextureLoad("Assets/Switch_It_Up.png");
	buffPromptFont = AEGfxCreateFont("Assets/m04.ttf", BUFF_PROMPT_FONT_SIZE);
}
void BuffCardScreen::Update() {
	const float FLIP_SPEED = 5.0f; // Adjust flip speed

	for (int i = 0; i < NUM_CARDS; ++i) {
		if (cardFlipping[i]) {
			// Animate from -1.0 (back) to 1.0 (front)
			cardFlipStates[i] += static_cast<f32>(FLIP_SPEED * AEFrameRateControllerGetFrameTime())	;

			if (cardFlipStates[i] >= 1.0f) {
				cardFlipStates[i] = 1.0f;
				cardFlipping[i] = false; // Flip complete
			}
		}
	}
	// Automated sequential flipping
	if (!allCardsFlipped) {
		// Check if we need to start flipping the next card
		if (currentFlipIndex < NUM_CARDS) {
			// Create timer for this card if not created yet
			std::string timerName = "Flip Timer " + std::to_string(currentFlipIndex);

			if (!flipTimerCreated[currentFlipIndex]) {
				float delay = 0.45f; // Delay between cards
				TimerSystem::GetInstance().AddTimer(timerName, delay, false);
				flipTimerCreated[currentFlipIndex] = true;
			}

			auto* timer = TimerSystem::GetInstance().GetTimerByName(timerName);
			// Check if timer completed
			if (timer &&
				timer->completed) {
				FlipCard(currentFlipIndex);
				TimerSystem::GetInstance().RemoveTimer(timerName);
				currentFlipIndex++; // Move to next card
			}
		}
		else {
			allCardsFlipped = true; // All cards done
		}
	}
	if (AEInputCheckTriggered(AEVK_P)) { // Remember to set textloading back to false for fadeonce flag.
		ResetFlipSequence();
	}
}
// This function resets the flip, simulating a shuffle.
// TODO : The buff card type and rarity should be randomized during this function.
void BuffCardScreen::ResetFlipSequence()
{
	// Reset animation state
	for (int i = 0; i < NUM_CARDS; ++i)
	{
		cardFlipStates[i] = -1.0f;     // Back side again
		cardFlipping[i] = false;
		flipTimerCreated[i] = false;

		// Remove any existing timers
		std::string timerName = "Flip Timer " + std::to_string(i);
		TimerSystem::GetInstance().RemoveTimer(timerName);
	}

	currentFlipIndex = 0;
	allCardsFlipped = false;
}
void BuffCardScreen::FlipCard(int cardIndex) {
	if (cardIndex >= 0 && cardIndex < NUM_CARDS) {
		cardFlipStates[cardIndex] = -1.0f; // Start from back
		cardFlipping[cardIndex] = true;
	}
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
	// Rotation matrix.
	AEMtx33 rotate = { 0 };
	AEMtx33Rot(&rotate, 0);

	// Set render state
	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
	AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
	AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetTransparency(1.0f);

	const f32 CARD_SPACING = 450.f;
	const f32 CARD_SIZE_MODIFIER = 0.42f;
	//const f32 CARD_SIZE_SELECTED = 0.5f;

	for (int i = 0; i < NUM_CARDS; ++i) {
		float cardFlipProgress = cardFlipStates[i]; // -1.0 to 1.0
		AEMtx33 scale = { 0 };
		float scaleX = CARD_WIDTH * CARD_SIZE_MODIFIER * abs(cardFlipProgress);
		float scaleY = CARD_HEIGHT * CARD_SIZE_MODIFIER;
		AEMtx33Scale(&scale, scaleX, scaleY);

		// Determine which texture to use based on flip progress SIGN
		AEGfxTexture* currentTexture;
		if (cardFlipProgress < 0) {
			currentTexture = cardBackTex;
		}
		else {
			currentTexture = cardFrontTex[i];
		}

		f32 offsetX = (i - 1) * CARD_SPACING;

		AEMtx33 translate = { 0 };
		AEMtx33Trans(&translate,
			Camera::position.x * Camera::scale + offsetX,
			Camera::position.y * Camera::scale);

		AEMtx33 transform = { 0 };
		AEMtx33Concat(&transform, &rotate, &scale);
		AEMtx33Concat(&transform, &translate, &transform);

		// Set texture BEFORE drawing
		AEGfxTextureSet(currentTexture, 0, 0);

		AEGfxSetTransform(transform.m);
		AEGfxMeshDraw(cardMesh, AE_GFX_MDM_TRIANGLES);
	}
}
/*----------------------------

TODO: this function simulates discarding all cards for a shuffle buff, which a shuffle buff would call this function
then call drawbuffcards again after all cards have been cleared from screen.

---------------------------------*/
void BuffCardScreen::DiscardCards() {

}
void BuffCardScreen::Render() {
	DrawBlackOverlay();
	DrawPromptText();
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