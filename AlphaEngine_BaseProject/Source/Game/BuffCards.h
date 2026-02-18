#pragma once
#include "AEEngine.h"
#include <string>
#include <vector>

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
/*----------------------------------------------------------------------------
The BuffCardManager is responsible for setting up cards to save to JSON,
including the names and descriptions. The BuffCardScreen handles the actual
randomization of card types and rarities, and based on the card attained,
the BuffCardManager will apply the corresponding buffs to the player stats.
-----------------------------------------------------------------------------*/
class BuffCardManager {
public:
	static void Update();
	static std::vector<BuffCard> RandomizeCards(int numCards); // Returns an array of BuffCards with randomized types and rarities.
	static CARD_TYPE GetCardType(std::string str); // Get card type from string, used for loading from JSON.
	static CARD_RARITY GetCardRarity(std::string str); // Get card rarity from string, used for loading from JSON.
	static void SaveCardInfo(); // Save card names and descriptions to a json file.
	static void LoadCardInfo(); // Load card names and descriptions from a json file.
	inline static std::string CardTypeToString(CARD_TYPE type) {
		switch (type) {
		case HERMES_FAVOR: return "HERMES_FAVOR";
		case IRON_DEFENCE: return "IRON_DEFENCE";
		case SWITCH_IT_UP: return "SWITCH_IT_UP";
		case REVITALIZE: return "REVITALIZE";
		case SHARPEN: return "SHARPEN";
		default: return "UNKNOWN_TYPE";
		}
	}

	// Convert CARD_RARITY to string
	inline static std::string CardRarityToString(CARD_RARITY rarity) {
		switch (rarity) {
		case RARITY_UNCOMMON: return "UNCOMMON";
		case RARITY_RARE: return "RARE";
		case RARITY_EPIC: return "EPIC";
		case RARITY_LEGENDARY: return "LEGENDARY";
		default: return "UNKNOWN_RARITY";
		}
	}

private:
	static const int UNCOMMON_CARDS = 2; // Number of uncommon cards in the pool.
	static const int RARE_CARDS = 2; // Number of rare cards in the pool.
	static const int EPIC_CARDS = 2; // Number of epic cards in the pool.
	static const int LEGENDARY_CARDS = 1; // Number of legendary cards in the pool.

	// Flags
	inline static bool shuffled = false; // To ensure the card shuffle only occurs once per call.

	inline static int cardSelected = 0; // Current selected card.
	inline static std::vector<BuffCard> currentCards; // Store current cards for reference in rendering and effects.

	inline static const std::string file = "buff-cards-info.json"; // File to save card info to.
};
/*----------------------------------------------------------------------------
The BuffCardScreen is responsible for rendering the card drawing screen, 
including the black overlay, prompt text, and the cards themselves with flip 
animation. It also handles the reset of the card states for simulating a shuffle 
visually when the player obtains a shuffle buff, allowing the player to draw 
new cards.
-----------------------------------------------------------------------------*/
class BuffCardScreen {
public:
	static void Init();
	static void Update();
	static void Render();
	static void Exit();



	// Standardise to 3 buffs for drawing to screen. Boss guarantees legendary buffs.
	inline static const int NUM_CARDS = 3;
	// Black overlay for card drawing.
	static void DrawBlackOverlay();

	// Prompt text for card drawing.
	static void DrawPromptText();
	// Draw the cards with flip animation.
	static void DrawDeck();
	static void FlipCard(int cardIndex); // Trigger flip animation

	// Reset flip states and timers to simulate a shuffle, allowing cards to be drawn again.
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
	static const int UNIQUE_CARD_TEXTURES = 3; // Total number of unique card textures available (for different types and rarities).
	inline static AEGfxTexture* cardFrontTex[UNIQUE_CARD_TEXTURES] = { nullptr }; // 3 different front textures
	inline static AEGfxVertexList* cardMesh = nullptr;

	// Card visual attributes.
	inline static f32 cardFlipStates[3] = { -1.0f, -1.0f, -1.0f }; // Start showing backs
	inline static bool cardFlipping[3] = { false, false, false };
	inline static int currentFlipIndex = 0;
	inline static bool allCardsFlipped = false;
	inline static bool flipTimerCreated[3] = { false, false, false };

	// Constant values
	static const int CARD_WIDTH = 750;
	static const int CARD_HEIGHT = 1050;
};