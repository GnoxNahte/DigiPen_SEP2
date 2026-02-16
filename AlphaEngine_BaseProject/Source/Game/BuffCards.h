#pragma once
#include "AEEngine.h"
#include <string>

// Enumeration types for card type.
enum CARD_TYPE {
	HERMES_FAVOR,
	TOUGH_SKIN,
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
	static void Render();
	static void Exit();
	static void RandomizeCards();
	static void DrawBlackOverlay();
	static void DrawPromptText();
	static void DrawDeck();
	//static void DrawBuffCards();
	static void DiscardCards();
	inline AEGfxTexture* GetCardBackTexture() const { return cardBackTex; }
	inline AEGfxVertexList* GetCardMesh() const { return cardMesh; }
private:

	// Flags 
	inline static bool textLoading = false; // To ensure text fading only occurs once.
	inline static bool shuffling = false; // To ensure the card shuffle happens once per call.

	// Mesh for black transparent rectangle overlay.
	inline static AEGfxVertexList* rectMesh = nullptr;

	// Font for cards and tooltips
	inline static s8 buffPromptFont;
	static const int BUFF_PROMPT_FONT_SIZE = 36;

	// Card texture and mesh
	inline static AEGfxTexture* cardBackTex = nullptr;
	inline static AEGfxVertexList* cardMesh = nullptr;

	// Card sprite dimensions
	static const int CARD_WIDTH = 750;
	static const int CARD_HEIGHT = 1050;
};