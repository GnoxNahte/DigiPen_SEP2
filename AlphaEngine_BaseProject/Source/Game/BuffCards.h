#pragma once
#include "AEEngine.h"
#include <string>

// Enumeration types for card type.
enum CARD_TYPE {
	HERMES_FAVOR,
	IRON_DEFENCE,
	SWITCH_IT_UP,
	REVITALIZE,
	SHARPEN
};

// Enumeration types for card rarity.
enum CARD_RARITY {
	RARITY_UNCOMMON,
	RARITY_RARE,
	RARITY_EPIC,
	RARITY_LEGENDARY
};
struct BuffCard {
	CARD_RARITY rarity{};
	CARD_TYPE type{};
	std::string cardName{};
	std::string cardDesc{};
	AEVec2 cardPos{};
	BuffCard(CARD_RARITY cr = RARITY_UNCOMMON, // Constructor
			 CARD_TYPE ct = HERMES_FAVOR,
			 std::string cName = "Unnamed Card",
			 std::string cDesc = "No description provided.",
			 AEVec2 pos = { 0,0 });

};


class BuffCardScreen {
public:
	static void Init();
	static void Update();
	static void Render();
	static void Exit();
	//static void RandomizeCards();
	static void DrawBlackOverlay();
	static void DrawPromptText();
	static void DrawDeck();
	static void FlipCard(int cardIndex); // Trigger flip animation
	static void ResetFlipSequence();
	static void DiscardCards();
	inline AEGfxTexture* GetCardBackTexture() const { return cardBackTex; }
	inline AEGfxVertexList* GetCardMesh() const { return cardMesh; }
private:

	// Flags 
	inline static bool textLoading = false; // To ensure text fading only occurs once.
	inline static bool flipped = false; // To ensure the card flips only occurs once per call.

	// Mesh for black transparent rectangle overlay.
	inline static AEGfxVertexList* rectMesh = nullptr;

	// Font for cards and tooltips
	inline static s8 buffPromptFont;
	static const int BUFF_PROMPT_FONT_SIZE = 36;

	// Card texture and mesh
	inline static AEGfxTexture* cardBackTex = nullptr;
	inline static AEGfxTexture* cardFrontTex[3] = { nullptr }; // 3 different front textures
	inline static AEGfxVertexList* cardMesh = nullptr;

	// Card attributes.
	inline static int cardSelected = 0;
	inline static f32 cardFlipStates[3] = { -1.0f, -1.0f, -1.0f }; // Start showing backs
	inline static bool cardFlipping[3] = { false, false, false };
	inline static int currentFlipIndex = 0;
	inline static bool allCardsFlipped = false;
	inline static bool flipTimerCreated[3] = { false, false, false };

	// Constant values
	static const int CARD_WIDTH = 750;
	static const int CARD_HEIGHT = 1050;
	static const int NUM_CARDS = 3;
};