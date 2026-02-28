#pragma once
#include "AEEngine.h"
#include "AETypes.h"
#include <string>
#include <vector>

// Enumeration types for card type.
enum CARD_TYPE {
	HERMES_FAVOR,
	IRON_DEFENCE,
	SWITCH_IT_UP,
	REVITALIZE,
	SHARPEN,
	BERSERKER,
	FEATHERWEIGHT
};

// Enumeration types for card rarity.
enum CARD_RARITY {
	RARITY_UNCOMMON,
	RARITY_RARE,
	RARITY_EPIC,
	RARITY_LEGENDARY
};

// Standardise to 3 buffs for drawing to screen.
inline static const int NUM_CARDS = 3;

struct RarityThreshold {
	CARD_RARITY rarity;
	float threshold; // cumulative probability (0.0 - 1.0)
};

// Define thresholds for rarity randomization. 
// The probabilities are cumulative, so they should be in ascending 
// order and the last threshold should be 1.0.
inline static const RarityThreshold rarityTable[] = {
	{ RARITY_UNCOMMON, 0.50f }, // 50% chance for uncommon
	{ RARITY_RARE, 0.85f }, // 35% chance for rare (0.85 - 0.50)
	{ RARITY_EPIC, 0.95f }, // 10% chance for epic (0.95 - 0.85)
	{ RARITY_LEGENDARY, 1.0f } // 5% chance for legendary (1.0 - 0.95)
};

struct BuffCard {
	CARD_RARITY rarity{};
	CARD_TYPE type{};
	std::string cardName{};
	std::string cardDesc{};
	std::string cardEffect{};
	int effectValue1{}, effectValue2{};
	bool selected{}; // Numerical values representing the strength of the buff (percentage-based).
	AEVec2 cardPos{};
	BuffCard(CARD_RARITY cr = RARITY_UNCOMMON, // Constructor
			 CARD_TYPE ct = HERMES_FAVOR,
			 std::string cName = "Unnamed Card",
			 std::string cDesc = "No description provided.",
			 std::string cEffect = "No effect.",
			 int eValue1 = 0,
			 int eValue2 = 0,
			 bool slcted = false,
			 AEVec2 pos = { 0,0 });

};

struct BuffSelectedEvent {
	const BuffCard& card;
};

/*----------------------------------------------------------------------------
The BuffCardManager is responsible for setting up cards to save to JSON,
including the names and descriptions. The BuffCardScreen handles the actual
randomization of card types and rarities, and based on the card attained,
the BuffCardManager will apply the corresponding buffs to the player stats.
-----------------------------------------------------------------------------*/
class BuffCardManager {
public:
	static void Init();
	static void Update();
	static void RandomizeCards(int numCards); // Returns an array of BuffCards with randomized types and rarities.
	static CARD_RARITY DetermineRarity(); // Determine card rarity based on a random roll and predefined thresholds.
	static CARD_TYPE DetermineType(CARD_RARITY rarity); // Determine card rarity based on a random roll and predefined thresholds.
	static void ApplyCardEffect(const BuffCard& card); // Apply the effect of the selected card to the player stats.

	// Getter functions for the card pools based on rarity, used for randomization and rendering.
	inline static const std::vector<BuffCard> GetUncommonCards() { return uncommonCards; }
	inline static const std::vector<BuffCard> GetRareCards() { return rareCards; }
	inline static const std::vector<BuffCard> GetEpicCards() { return epicCards; }
	inline static const std::vector<BuffCard> GetLegendaryCards() { return legendaryCards; }
	inline static const std::vector<BuffCard> GetRandomizedCards() { return randomizedCards; }
	// Get the current buffs the player has, for reference when applying new buffs and for displaying in the UI.
	inline static const std::vector<BuffCard>& GetCurrentBuffs() { return currentBuffs; }
	inline static void AddBuff(const BuffCard& card) { currentBuffs.push_back(card); }


	static void SelectCards(std::vector<BuffCard>& cards); // Handle player input for selecting a card and applying its effect.
	inline static bool& IsCardSelected() { return firstCardSelected; }
	inline static int& CurrentSelectedCard() { return cardSelected; }
	inline static bool& IsCardSelectedThisUpdate() { return cardSelectedThisUpdate; }
	inline static bool& IsRoomCleared() { return roomCleared; } // To allow card selection to occur only after clearing a room.
	/*--------------------------------------------------------------------------
							File Reading and Writing
	--------------------------------------------------------------------------*/
	static CARD_TYPE GetCardType(std::string str); // Get card type from string, used for loading from JSON.
	static CARD_RARITY GetCardRarity(std::string str); // Get card rarity from string, used for loading from JSON.
	static void LoadCardInfo(); // Load card names and descriptions from a json file.
	// Convert CARD_TYPE to string
	inline static std::string CardTypeToString(CARD_TYPE type) {
		switch (type) {
		case HERMES_FAVOR: return "HERMES_FAVOR";
		case IRON_DEFENCE: return "IRON_DEFENCE";
		case SWITCH_IT_UP: return "SWITCH_IT_UP";
		case REVITALIZE: return "REVITALIZE";
		case SHARPEN: return "SHARPEN";
		case BERSERKER: return "BERSERKER";
		case FEATHERWEIGHT: return "FEATHERWEIGHT";
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
	static const int UNCOMMON_CARDS = 20; // Number of uncommon cards in the pool.
	static const int RARE_CARDS = 20; // Number of rare cards in the pool.
	static const int EPIC_CARDS = 20; // Number of epic cards in the pool.
	static const int LEGENDARY_CARDS = 10; // Number of legendary cards in the pool.

	// Flags
	inline static bool shuffled = false; // To ensure the card shuffle only occurs once per call.
	inline static bool firstCardSelected = false; // Flag to ensure the delay timer is only added once for the first card selection.
	// Flag to track if the current room has been cleared, allowing card selection to occur only after clearing a room.
	inline static bool roomCleared = false;
	// Flag to track if a card has been selected in the current update cycle, preventing multiple selections in one update.
	inline static bool cardSelectedThisUpdate = false;
	// Flag to initialize mouse position on initial use of mouse.
	inline static bool mousePosInitialized = false;

	inline static int cardSelected = 0; // Current selected card.

	// Store the current set of randomized cards for rendering and applying effects.
	inline static std::vector<BuffCard> randomizedCards;

	//Store all cards read from file to feed into different rarity vectors.
	inline static std::vector<BuffCard> allCards;

	// Different rarity vectors to determine what card is attained based on the rarity rolled.
	inline static std::vector<BuffCard> uncommonCards;
	inline static std::vector<BuffCard> rareCards;
	inline static std::vector<BuffCard> epicCards;
	inline static std::vector<BuffCard> legendaryCards;

	// Store the current buffs the player has, to be displayed in the UI and for reference when applying new buffs.
	inline static std::vector<BuffCard> currentBuffs{};


	inline static const std::string file = "buff-cards-pool.json"; // File to save card info to.
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

	struct BuffCardRect {
		AEVec2 pos;
		AEVec2 size;
	};
	inline static std::vector<BuffCardRect> cachedCardRects{};

	// Black overlay for card drawing.
	static void DrawBlackOverlay();
	// Prompt text for card drawing.
	static void DrawPromptText(const std::vector<BuffCard>& cards, int selectedIdx);
	// Draw the cards with flip animation.
	static void DrawDeck(const std::vector<BuffCard> cards);
	static void FlipCard(int cardIndex); // Trigger flip animation
	//static void DrawCardDesc(const BuffCard& card); // Draw the description of the card when flipped.

	// Reset flip states and timers to simulate a shuffle, allowing cards to be drawn again.
	static void ResetFlipSequence();
	inline AEGfxTexture* GetCardBackTexture() const { return cardBackTex; }
	inline AEGfxVertexList* GetCardMesh() const { return cardMesh; }
	inline static const bool GetCardsFlipStatus() { return allCardsFlipped; } // Check if all cards have been flipped to show fronts and descriptions.
	inline static const std::vector<f32> GetCardFlipStates() { return cardFlipStates; }
	inline static const int GetCurrentFlipIndex() { return currentFlipIndex; }
	inline static bool& GetTextLoadingStatus() { return textLoading; } // To allow fade in of text only once for each card draw.
private:

	// Flags 
	inline static bool textLoading = false; // To ensure text fading only occurs once.
	inline static bool flipped = false; // To ensure the card flips only occurs once per call.

	// Mesh for black transparent rectangle overlay.
	inline static AEGfxVertexList* rectMesh = nullptr;

	// Font for cards and tooltips
	inline static s8 buffPromptFont;
	inline static s8 cardBuffFont;
	static const int BUFF_PROMPT_FONT_SIZE = 36;
	static const int CARD_BUFF_FONT_SIZE = 32;

	// Card texture and mesh
	inline static AEGfxTexture* cardBackTex = nullptr;
	static const int UNIQUE_CARD_TEXTURES = 20; // Total number of unique card textures available (for different types and rarities).
	inline static AEGfxTexture* cardFrontTex[UNIQUE_CARD_TEXTURES] = { nullptr }; // 5 different front textures
	static const int UNIQUE_RARITY_TEXTURES = 4; // Total number of unique rarity textures available (for different rarities).
	inline static AEGfxTexture* cardRarityTex[UNIQUE_RARITY_TEXTURES] = { nullptr }; // 4 different rarities
	inline static AEGfxVertexList* cardMesh = nullptr;

	// Card visual attributes.
	inline static std::vector<f32> cardFlipStates { -1.0f, -1.0f, -1.0f }; // Start showing backs
	inline static f32 cardYOffset[NUM_CARDS] = { 0 }; // 0 = normal
	inline static bool cardFlipping[3] = { false, false, false };
	inline static int currentFlipIndex = 0;
	inline static bool allCardsFlipped = false;
	inline static bool flipTimerCreated[3] = { false, false, false };
	inline static const float FLIP_SPEED = 10.5f; // Adjust flip speed

	// Constant values
	static const int CARD_WIDTH = 750;
	static const int CARD_HEIGHT = 1050;

	// Black overlay attributes.
	inline static f32 overlayAlpha = 0.75f;
	inline static f32 fadeSpeed = 3.5f;
};