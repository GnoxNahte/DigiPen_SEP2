//#include <iostream>
#include <ctime>
#include <filesystem>
#include <rapidjson/document.h>
#include "../Utils/MeshGenerator.h"
#include "../Utils/AEExtras.h"
#include "../Utils/FileHelper.h"
#include "../Game/UI.h"
#include "BuffCards.h"
#include "Camera.h"
#include "Timer.h"

BuffCard::BuffCard( // Constructor
	CARD_RARITY cr,
	CARD_TYPE ct,
	std::string cName,
	std::string cDesc,
	std::string cEffect,
	int eValue1,
	int eValue2,
	AEVec2 pos) :
	rarity{ cr }, type{ ct }, cardName{ cName }, cardDesc{ cDesc }, cardEffect{ cEffect }, 
	effectValue1 {eValue1}, effectValue2{ eValue2 }, cardPos{ pos }
	{/* empty by design */}

void BuffCardManager::Init() {
	LoadCardInfo();
}

void BuffCardManager::Update() {
	if (!shuffled && AEInputCheckTriggered(AEVK_P)) {
		BuffCardManager::RandomizeCards(BuffCardScreen::NUM_CARDS);
		//SaveCardInfo();
		//LoadCardInfo();
		//shuffled = true;
	}
}

CARD_RARITY BuffCardManager::DetermineRarity() {
	f32 rarityRoll = AEExtras::RandomRange({ 0.f, 1.f }); // Get a random float between 0 and 1 to determine rarity.
	for (const RarityThreshold entry : rarityTable) {
		if (rarityRoll < entry.threshold) {
			std::cout << "Determined Rarity: " << CardRarityToString(entry.rarity) << " for roll: " << rarityRoll << std::endl;
			return entry.rarity;
		}
	}
	return RARITY_UNCOMMON; // default fallback, should never reach here if thresholds are set correctly.
}

CARD_TYPE BuffCardManager::DetermineType(CARD_RARITY rarity) {
	std::vector<BuffCard> cardPool;
	if (rarity == RARITY_UNCOMMON) {
		cardPool = GetUncommonCards();
	} else if (rarity == RARITY_RARE) {
		cardPool = GetRareCards();
	} else if (rarity == RARITY_EPIC) {
		cardPool = GetEpicCards();
	} else if (rarity == RARITY_LEGENDARY) {
		cardPool = GetLegendaryCards();
	}
	if (cardPool.empty()) {
		std::cout << "CARD POOL IS EMPTY !!!";
		return HERMES_FAVOR; // Fallback if pool is empty
	}

	// Generate random float [0, 1)
	f32 cardTypeRoll = AEExtras::RandomRange({ 0.f, 1.f });

	// Map roll to an index in cardPool
	size_t index = static_cast<size_t>(cardTypeRoll * cardPool.size());

	// Clamp index to valid range (in case roll == 1.0)
	if (index >= cardPool.size()) {
		index = cardPool.size() - 1;
	}
	return cardPool[index].type;
}

/*-------------------------------------------------------------
Randomize card types and rarities for the current set of cards.
Called once per shuffle.
--------------------------------------------------------------*/
void BuffCardManager::RandomizeCards(int numCards) {
	for (int i = 0; i < numCards; ++i) {

		/*--------------------------------------------------------------------
		Handling of card rarity. Rarity determines the strength of the buffs
		and the pool of possible buffs.
		----------------------------------------------------------------------*/
		CARD_RARITY rarity{ DetermineRarity() };
		/*--------------------------------------------------------------------
		Handling of card type. Some card types only appear in certain rarities,
		such as legendary cards only having access to the most powerful buffs.
		----------------------------------------------------------------------*/
		CARD_TYPE type{ DetermineType(rarity) };

		// Create card name and description based on type and rarity (placeholder logic).
		std::string cardName = "Card " + std::to_string(i + 1);
		std::string cardDesc = "A mysterious buff.";
		std::string cardEffect = "Effect goes here.";
		randomizedCards.push_back(BuffCard(rarity, type, cardName, cardDesc, cardEffect));
	}
	/*----------------------------------------------------------------------------------------------------
	Checks if the randomization is working correctly by printing out the randomized cards to the console.
	----------------------------------------------------------------------------------------------------*/
	for (int i = 0; i < numCards; ++i) {
		std::cout << "RANDOMIZED CARD " << i  << "  "  << randomizedCards[i].cardName << " - Rarity: " << BuffCardManager::CardRarityToString(randomizedCards[i].rarity)
			<< ", Type: " << BuffCardManager::CardTypeToString(randomizedCards[i].type) << "Desc: " << randomizedCards[i].cardDesc << "Effect: " 
			<< randomizedCards[i].cardEffect << std::endl;
	}
}

CARD_TYPE BuffCardManager::GetCardType(std::string typeStr) {
	if (typeStr == "HERMES_FAVOR")  return HERMES_FAVOR;
	else if (typeStr == "IRON_DEFENCE") return IRON_DEFENCE;
	else if (typeStr == "SWITCH_IT_UP") return SWITCH_IT_UP;
	else if (typeStr == "REVITALIZE") return REVITALIZE;
	else if (typeStr == "SHARPEN") return SHARPEN;
	else return HERMES_FAVOR; // default fallback
}
CARD_RARITY BuffCardManager::GetCardRarity(std::string rarityStr) {
	if (rarityStr == "UNCOMMON") return RARITY_UNCOMMON;
	else if (rarityStr == "RARE") return RARITY_RARE;
	else if (rarityStr == "EPIC") return RARITY_EPIC;
	else if (rarityStr == "LEGENDARY") return RARITY_LEGENDARY;
	else return RARITY_UNCOMMON; // default fallback
}

void BuffCardManager::LoadCardInfo() {
	allCards.clear(); // clear previous cards

	// Build the path to the JSON file
	std::string actualAssetPath = "../../Assets/config/" + file;

	if (!std::filesystem::exists(actualAssetPath)) {
		std::cout << "Card info file does not exist: " << actualAssetPath << std::endl;
		return; // nothing to load
	}

	rapidjson::Document doc;
	if (!FileHelper::TryReadJsonFile(actualAssetPath, doc)) {
		std::cout << "Failed to read card info file." << std::endl;
		return;
	}

	if (!doc.HasMember("cards") || !doc["cards"].IsArray()) {
		std::cout << "Invalid card info format." << std::endl;
		return;
	}

	const rapidjson::Value& cardArray = doc["cards"];
	for (rapidjson::SizeType i = 0; i < cardArray.Size(); i++) {
		const auto& cardObj = cardArray[i];
		BuffCard card;

		// Read type and rarity as integers
		if (cardObj.HasMember("type") && cardObj["type"].IsString()) {
			card.type = GetCardType(cardObj["type"].GetString());
		}

		if (cardObj.HasMember("rarity") && cardObj["rarity"].IsString()) {
			card.rarity = GetCardRarity(cardObj["rarity"].GetString());
		}

		// Read name and description
		if (cardObj.HasMember("name") && cardObj["name"].IsString()) {
			card.cardName = cardObj["name"].GetString();
		}

		if (cardObj.HasMember("description") && cardObj["description"].IsString()) {
			card.cardDesc = cardObj["description"].GetString();
		}
		if (cardObj.HasMember("effect") && cardObj["effect"].IsString()) {
			card.cardEffect = cardObj["effect"].GetString();
		}
		if (cardObj.HasMember("effect_value_1") && cardObj["effect_value_1"].IsInt()) {
			card.effectValue1 = cardObj["effect_value_1"].GetInt();
		}
		if (cardObj.HasMember("effect_value_2") && cardObj["effect_value_2"].IsInt()) {
			card.effectValue2 = cardObj["effect_value_2"].GetInt();
		}

		allCards.push_back(card);

		if (card.rarity == RARITY_UNCOMMON) {
			uncommonCards.push_back(card);
		}
		else if (card.rarity == RARITY_RARE) {
			rareCards.push_back(card);
		}
		else if (card.rarity == RARITY_EPIC) {
			epicCards.push_back(card);
		}
		else if (card.rarity == RARITY_LEGENDARY) {
			legendaryCards.push_back(card);
		}

		//std::cout << "Loaded card: " << card.cardName << " Type: " << BuffCardManager::CardTypeToString(card.type) << ", Rarity: " 
		//	<< BuffCardManager::CardRarityToString(card.rarity) << "\nDescription: " << card.cardDesc << "\nEffect: " << card.cardEffect 
		//	<<" Effect value 1: "  << card.effectValue1 << " Effect value 2:" << card.effectValue2 << std::endl;
	}

	std::cout << "Loaded " << allCards.size() << " cards from JSON." << std::endl;
}

void BuffCardScreen::Init() {
	rectMesh = MeshGenerator::GetRectMesh(1.0f, 1.0f);
	cardMesh = MeshGenerator::GetRectMesh(1.0f, 1.0f);
	cardBackTex = AEGfxTextureLoad("Assets/0_CardBack.png");
	cardFrontTex[HERMES_FAVOR] = AEGfxTextureLoad("Assets/Hermes_Favor.png");
	cardFrontTex[IRON_DEFENCE] = AEGfxTextureLoad("Assets/Iron_Defence.png");
	cardFrontTex[SWITCH_IT_UP] = AEGfxTextureLoad("Assets/Switch_It_Up.png");
	cardFrontTex[REVITALIZE] = AEGfxTextureLoad("Assets/Revitalize.png");
	buffPromptFont = AEGfxCreateFont("Assets/m04.ttf", BUFF_PROMPT_FONT_SIZE);
	srand(static_cast<unsigned int>(time(NULL))); // Seed random number generator with current time for variability.
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
	if (AEInputCheckTriggered(AEVK_L)) { // Remember to set textloading back to false for fadeonce flag.
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
void BuffCardScreen::DrawDeck(const std::vector<BuffCard> cards) {
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

	for (int i = 0; i < cards.size(); ++i) {
		float cardFlipProgress = cardFlipStates[i]; // -1.0 to 1.0
		AEMtx33 scale = { 0 };
		float scaleX = CARD_WIDTH * CARD_SIZE_MODIFIER * abs(cardFlipProgress);
		float scaleY = CARD_HEIGHT * CARD_SIZE_MODIFIER;
		AEMtx33Scale(&scale, scaleX, scaleY);

		// Determine which texture to use based on flip progress SIGN
		AEGfxTexture* currentTexture = nullptr;
		if (cardFlipProgress < 0) {
			currentTexture = cardBackTex;
		}
		else {
			// Use the card's type to get the texture
			currentTexture = cardFrontTex[cards[i].type];
		}

		f32 offsetX = (i - 1) * CARD_SPACING; // adjust depending on how you center cards

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
	DrawDeck(BuffCardManager::GetRandomizedCards());
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