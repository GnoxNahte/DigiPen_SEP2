//#include <iostream>
#include <ctime>
#include <filesystem>
#include <rapidjson/document.h>
#include "../Utils/MeshGenerator.h"
#include "../Utils/AEExtras.h"
#include "../Utils/FileHelper.h"
#include "../Game/UI.h"
#include "Time.h"
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
	bool slcted,
	AEVec2 pos) :
	rarity{ cr }, type{ ct }, cardName{ cName }, cardDesc{ cDesc }, cardEffect{ cEffect }, 
	effectValue1{ eValue1 }, effectValue2{ eValue2 }, selected{ slcted }, cardPos {pos}
	{/* empty by design */}

void BuffCardManager::Init() {
	LoadCardInfo();
	cardSelected = 0; // Initialize selected card index to 0 at the start of selection.
}
void BuffCardScreen::Init() {
	rectMesh = MeshGenerator::GetRectMesh(1.0f, 1.0f);
	cardMesh = MeshGenerator::GetRectMesh(1.0f, 1.0f);

	// Card assets
	cardBackTex = AEGfxTextureLoad("Assets/0_CardBack.png");
	cardFrontTex[HERMES_FAVOR] = AEGfxTextureLoad("Assets/Hermes_Favor.png");
	cardFrontTex[IRON_DEFENCE] = AEGfxTextureLoad("Assets/Iron_Defence.png");
	cardFrontTex[SWITCH_IT_UP] = AEGfxTextureLoad("Assets/Switch_It_Up.png");
	cardFrontTex[REVITALIZE] = AEGfxTextureLoad("Assets/Revitalize.png");
	cardFrontTex[SHARPEN] = AEGfxTextureLoad("Assets/Sharpen.png");
	cardFrontTex[BERSERKER] = AEGfxTextureLoad("Assets/Berserker.png");
	cardFrontTex[FEATHERWEIGHT] = AEGfxTextureLoad("Assets/Featherweight.png");

	cardRarityTex[RARITY_UNCOMMON] = AEGfxTextureLoad("Assets/Uncommon_Emission.png");
	cardRarityTex[RARITY_RARE] = AEGfxTextureLoad("Assets/Rare_Emission.png");
	cardRarityTex[RARITY_EPIC] = AEGfxTextureLoad("Assets/Epic_Emission.png");
	cardRarityTex[RARITY_LEGENDARY] = AEGfxTextureLoad("Assets/Legendary_Emission.png");

	buffPromptFont = AEGfxCreateFont("Assets/m04.ttf", BUFF_PROMPT_FONT_SIZE);
	cardBuffFont = AEGfxCreateFont("Assets/Pixellari.ttf", CARD_BUFF_FONT_SIZE);
	srand(static_cast<unsigned int>(time(NULL))); // Seed random number generator with current time for variability.
}

void BuffCardManager::Update() {
	if (!shuffled && roomCleared) { // Randomize cards only once per shuffle event, controlled by the shuffled flag.
		BuffCardManager::RandomizeCards(BuffCardScreen::NUM_CARDS);
		shuffled = true;
	}
	// Handle player input for card selection and applying effects. 
	// Called every update to check for input, but will only apply effect 
	// once per selection due to the cardSelectedThisUpdate flag.
	SelectCards(randomizedCards);
}
void BuffCardManager::ApplyCardEffect(const BuffCard& card) {
	if (card.type == SWITCH_IT_UP) {
		// If the card effect is "Switch It Up", trigger a shuffle by resetting the flip sequence and allowing new cards to be drawn.
		BuffCardScreen::ResetFlipSequence();
		shuffled = false; // Reset shuffled flag to allow randomization of new cards in the next update cycle.
	}
	BuffCardScreen::GetTextLoadingStatus() = false;

}
void BuffCardManager::SelectCards(std::vector<BuffCard>& cards) {
	if (cards.empty()) {
		return;
	}

	// Use a small positive value (0.1) so the card has visual "meat" before selection
	const float INTERACTION_THRESHOLD = 0.1f;

	// Check the progress of the CURRENTLY active card in the sequence
	// instead of just index 0.
	int currentIndex = BuffCardScreen::GetCurrentFlipIndex();

	// Safety check for bounds
	if (currentIndex >= cards.size()) {
		currentIndex = (int)cards.size() - 1;
	}

	float progress = BuffCardScreen::GetCardFlipStates().at(currentIndex);

	if (progress >= INTERACTION_THRESHOLD && !firstCardSelected) {
		// Automatically select the first card that becomes visible
		cards[0].selected = true;
		firstCardSelected = true;
	}
	if (firstCardSelected) {
		if (AEInputCheckTriggered(AEVK_RIGHT) || AEInputCheckTriggered(AEVK_D)) {
			cardSelected = static_cast<int>((cardSelected + 1) % cards.size()); // Move selection right, wrap around
			// Deselect all cards first
			for (BuffCard& c : cards) {
				c.selected = false;
			}
		}
		else if (AEInputCheckTriggered(AEVK_LEFT) || AEInputCheckTriggered(AEVK_A)) {
			cardSelected = static_cast<int>((cardSelected - 1 + cards.size()) % cards.size()); // Move selection left, wrap around
			// Deselect all cards first
			for (BuffCard& c : cards) {
				c.selected = false;
			}
		}
		cards[cardSelected].selected = true;
		if (AEInputCheckTriggered(AEVK_SPACE) && !cardSelectedThisUpdate) {
			cardSelectedThisUpdate = true;
			ApplyCardEffect(cards[cardSelected]);
			Time::GetInstance().SetTimeScale(1.0f);
			// Apply card effect here. For now, just print the selected card to the console for testing.

			std::cout << "Selected Card: " << cards[cardSelected].cardName << " - Rarity: " << BuffCardManager::CardRarityToString(cards[cardSelected].rarity)
				<< ", Type:" << BuffCardManager::CardTypeToString(cards[cardSelected].type)  << " Effect: "
				<< cards[cardSelected].cardEffect << std::endl;
		}
	}

}
CARD_RARITY BuffCardManager::DetermineRarity() {
	f32 rarityRoll = AEExtras::RandomRange({ 0.f, 1.f }); // Get a random float between 0 and 1 to determine rarity.
	for (const RarityThreshold entry : rarityTable) {
		if (rarityRoll < entry.threshold) {
			//std::cout << "Determined Rarity: " << CardRarityToString(entry.rarity) << " for roll: " << rarityRoll << std::endl;
			return entry.rarity;
		}
	}
	return RARITY_UNCOMMON; // default fallback, should never reach here if thresholds are set correctly.
}

CARD_TYPE BuffCardManager::DetermineType(CARD_RARITY rarity) {
	const std::vector<BuffCard>& cardPool = 
		(rarity == RARITY_UNCOMMON) ? uncommonCards :
		(rarity == RARITY_RARE) ? rareCards :
		(rarity == RARITY_EPIC) ? epicCards :
		legendaryCards;

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
	randomizedCards.clear();
	for (int i = 0; i < numCards; ++i) {

		/*--------------------------------------------------------------------
		Handling of card rarity. Rarity determines the strength of the buffs
		and the pool of possible buffs.
		----------------------------------------------------------------------*/
		CARD_RARITY rarity{ DetermineRarity() };

		const std::vector<BuffCard>* pool = nullptr;
		switch (rarity) {
		case RARITY_UNCOMMON: pool = &uncommonCards; break;
		case RARITY_RARE: pool = &rareCards; break;
		case RARITY_EPIC: pool = &epicCards; break;
		case RARITY_LEGENDARY: pool = &legendaryCards; break;
		}

		if (pool && !pool->empty()) {
			// Get a float [0,1)
			float roll = AEExtras::RandomRange({ 0.f, 1.f });

			// Convert to an index
			size_t index = static_cast<size_t>(roll * pool->size());

			// Clamp just in case roll == 1.0
			if (index >= pool->size()) index = pool->size() - 1;

			randomizedCards.push_back((*pool)[index]);
		}
	}
	/*----------------------------------------------------------------------------------------------------
	Checks if the randomization is working correctly by printing out the randomized cards to the console.
	----------------------------------------------------------------------------------------------------*/
	for (int i = 0; i < numCards; ++i) {
		//std::cout << "RANDOMIZED CARD " << i  << "  "  << randomizedCards[i].cardName << " - Rarity: " << BuffCardManager::CardRarityToString(randomizedCards[i].rarity)
			//<< ", Type: " << BuffCardManager::CardTypeToString(randomizedCards[i].type) << "Desc: " << randomizedCards[i].cardDesc << "Effect: " 
			//<< randomizedCards[i].cardEffect << std::endl;
	}
}

CARD_TYPE BuffCardManager::GetCardType(std::string typeStr) {
	if (typeStr == "HERMES_FAVOR")  return HERMES_FAVOR;
	else if (typeStr == "IRON_DEFENCE") return IRON_DEFENCE;
	else if (typeStr == "SWITCH_IT_UP") return SWITCH_IT_UP;
	else if (typeStr == "REVITALIZE") return REVITALIZE;
	else if (typeStr == "SHARPEN") return SHARPEN;
	else if (typeStr == "BERSERKER") return BERSERKER;
	else if (typeStr == "FEATHERWEIGHT") return FEATHERWEIGHT;
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

void BuffCardScreen::Update() {

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
	if (!allCardsFlipped && BuffCardManager::IsRoomCleared()) {
		// Check if we need to start flipping the next card
		if (currentFlipIndex < NUM_CARDS) {
			// Create timer for this card if not created yet
			std::string timerName = "Flip Timer " + std::to_string(currentFlipIndex);

			if (!flipTimerCreated[currentFlipIndex]) {
				float delay = 0.45f; // Delay between cards
				TimerSystem::GetInstance().AddTimer(timerName, delay, false, true);
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
	// Call this function every time we want to reshuffle and draw new cards, 
	// such as when the player picks "Switch It Up" or after a card selection is made.
	if (AEInputCheckTriggered(AEVK_L)) {
		ResetFlipSequence();
	}
	f32 dt = static_cast<f32>(AEFrameRateControllerGetFrameTime());

	if (BuffCardManager::IsCardSelectedThisUpdate())
	{
		// Fade OUT
		overlayAlpha -= fadeSpeed * dt;
	}
	else
	{
		// Fade IN
		overlayAlpha += fadeSpeed * dt;
	}

	// Clamp
	if (overlayAlpha < 0.f) overlayAlpha = 0.f;
	if (overlayAlpha > 0.85f) overlayAlpha = 0.85f;
}
// This function resets the flip, simulating a shuffle.
// TODO : The buff card type and rarity should be randomized during this function.
void BuffCardScreen::ResetFlipSequence()
{
	BuffCardManager::IsRoomCleared() = true;
	Time::GetInstance().SetTimeScale(0); // Pause time to prevent player interaction during shuffle
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
	BuffCardManager::IsCardSelected() = false;
	BuffCardManager::CurrentSelectedCard() = 0;
	BuffCardManager::IsCardSelectedThisUpdate() = false;

	// Randomize new cards
	BuffCardManager::RandomizeCards(NUM_CARDS);
}
void BuffCardScreen::FlipCard(int cardIndex) {
	if (cardIndex >= 0 && cardIndex < NUM_CARDS) {
		cardFlipStates[cardIndex] = -1.0f; // Start from back
		cardFlipping[cardIndex] = true;
	}
}
// Draw a black overlay when drawing cards.
void BuffCardScreen::DrawBlackOverlay()
{
	if (overlayAlpha <= 0.0f)
		return;

	AEMtx33 scale, rotate, translate, transform;

	AEMtx33Scale(&scale,
		static_cast<f32>(AEGfxGetWindowWidth() * 2),
		static_cast<f32>(AEGfxGetWindowHeight() * 2));

	AEMtx33Rot(&rotate, 0.0f);
	AEMtx33Trans(&translate,
		Camera::position.x * Camera::scale,
		Camera::position.y * Camera::scale);

	AEMtx33Concat(&transform, &rotate, &scale);
	AEMtx33Concat(&transform, &translate, &transform);

	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	AEGfxSetColorToMultiply(0.f, 0.f, 0.f, 0.f);
	AEGfxSetColorToAdd(0.f, 0.f, 0.f, overlayAlpha);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetTransparency(overlayAlpha);

	AEGfxSetTransform(transform.m);
	AEGfxMeshDraw(rectMesh, AE_GFX_MDM_TRIANGLES);
}
void BuffCardScreen::DrawPromptText(const std::vector<BuffCard>& cards, int selectedIdx) {
	if (!textLoading) {
		TimerSystem::GetInstance().AddTimer("Choose Buff Timer", 1.2f, true, true);
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
		// 1. Get the current selected card's data
		if (selectedIdx < 0 || selectedIdx >= cards.size()) return;
		const BuffCard& selected = cards[selectedIdx];

		// 2. Setup Base Coordinates (NDC)
		// We want the text at the bottom-center of the screen
		float baseTextX = -0.85f; // Left-ish start for alignment
		float titleY = -0.7f;    // Near the bottom
		float descY = -0.8f;    // Below the title
		float effectY = -0.9f;  // Below the description

		// 3. Draw "Choose a buff" Header
		// (Your existing code for this is fine as a global prompt)

		if (allCardsFlipped) {
			AEGfxPrint(buffPromptFont,
				selected.cardName.c_str(),
				baseTextX, titleY,
				1.0f,               // Scale
				1.0f, 1.0f, 1.0f,   // Color
				1.0f);              // Alpha

			AEGfxPrint(cardBuffFont,
				selected.cardDesc.c_str(),
				baseTextX, descY,
				0.8f,               // Slightly smaller scale for desc
				0.8f, 0.8f, 0.8f,   // Slightly dimmer white/grey
				1.0f);

			AEGfxPrint(cardBuffFont,
				selected.cardEffect.c_str(),
				baseTextX, effectY,
				0.8f,               // Slightly smaller scale for desc
				0.8f, 0.8f, 0.8f,   // Slightly dimmer white/grey
				1.0f);
		}
	}
}
// Draw buff cards.
void BuffCardScreen::DrawDeck(const std::vector<BuffCard> cards) {
	// Rotation matrix
	//AEMtx33 rotate = { 0 };
	//AEMtx33Rot(&rotate, 0);

	// Render state
	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
	AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
	AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetTransparency(1.0f);

	const f32 CARD_SPACING = 450.f;
	const f32 CARD_SIZE_MODIFIER = 0.42f;
	//const f32 CARD_SIZE_SELECTED = 0.46f;

	const float FLOAT_AMPLITUDE = 15.f;  // Vertical float
	const float FLOAT_SPEED = 4.f;       // Vertical oscillation speed
	const float SCALE_PULSE = 0.03f;     // Scale increase for pulse
	const float WOBBLE_AMPLITUDE = 8.f;  // Horizontal wobble
	const float WOBBLE_SPEED = 2.f;      // Horizontal wobble speed

	for (int i = 0; i < cards.size(); ++i) {
		float cardFlipProgress = cardFlipStates[i]; // -1.0 to 1.0
		float t = static_cast<float>(AEGetTime(nullptr));

		// Calculate how "active" the wobble should be.
		// (1.0f - abs(cardFlipProgress)) will be 1.0 when mid-flip (scaleX is 0)
		// and 0.0 when fully flat.
		float flipActivity = 1.0f - abs(cardFlipProgress);

		float scaleX = CARD_WIDTH * CARD_SIZE_MODIFIER * abs(cardFlipProgress);
		float scaleY = CARD_HEIGHT * CARD_SIZE_MODIFIER;
		f32 offsetX = (i - 1) * CARD_SPACING;
		f32 offsetY = 0.f;

		if (allCardsFlipped && cards[i].selected) {
			float currentWobbleSpeed = WOBBLE_SPEED;
			float currentWobbleAmp = WOBBLE_AMPLITUDE;

			if (cards[i].selected) {
				offsetY += sinf(t * FLOAT_SPEED) * FLOAT_AMPLITUDE;
				offsetX += sinf(t * currentWobbleSpeed) * currentWobbleAmp;

				scaleX *= (1.0f + SCALE_PULSE);
				scaleY *= (1.0f + SCALE_PULSE);
			}
			else {
				offsetX += sinf(t * FLOAT_SPEED + i) * (flipActivity * 10.0f);
				offsetY += cosf(t * FLOAT_SPEED + i) * (flipActivity * 5.0f);
			}
		}
		// --- ROTATION (The key to making it look natural) ---
		AEMtx33 rotate{ 0 };
		// Add a slight tilt that is strongest while the card is flipping
		float tiltAngle = flipActivity * 0.1f * sinf(t * 2.0f + i);
		AEMtx33Rot(&rotate, tiltAngle);

		AEMtx33 scale;
		AEMtx33Scale(&scale, scaleX, scaleY);

		AEGfxTexture* currentTexture = (cardFlipProgress < 0) ? cardBackTex : cardFrontTex[cards[i].type];

		AEMtx33 translate;
		AEMtx33Trans(&translate,
			Camera::position.x * Camera::scale + offsetX,
			Camera::position.y * Camera::scale + offsetY);

		AEMtx33 transform;
		AEMtx33Concat(&transform, &rotate, &scale);
		AEMtx33Concat(&transform, &translate, &transform);

		AEGfxTextureSet(currentTexture, 0, 0);
		AEGfxSetTransform(transform.m);
		AEGfxMeshDraw(cardMesh, AE_GFX_MDM_TRIANGLES);

		// --- Draw Rarity/Emission Overlay ---
		AEGfxTexture* emissionTex = cardRarityTex[cards[i].rarity];
		if (emissionTex) {
			AEMtx33 emissionScale;
			float EMISSION_SCALE = 1.15f; // 15% bigger than card
			AEMtx33Scale(&emissionScale, scaleX * EMISSION_SCALE, scaleY * EMISSION_SCALE);

			// Use same rotation and translation
			AEMtx33 emissionTransform;
			AEMtx33Concat(&emissionTransform, &rotate, &emissionScale);
			AEMtx33Concat(&emissionTransform, &translate, &emissionTransform);

			AEGfxTextureSet(emissionTex, 0, 0);
			AEGfxSetTransform(emissionTransform.m);
			AEGfxMeshDraw(cardMesh, AE_GFX_MDM_TRIANGLES);
		}
	}
}
// Draw the description of the card when flipped.
//void BuffCardScreen::DrawCardDesc(const BuffCard& card) {
//	
//}
void BuffCardScreen::Render() {
	if (!BuffCardManager::IsCardSelectedThisUpdate() && BuffCardManager::IsRoomCleared()) {
		DrawBlackOverlay();
		DrawPromptText(BuffCardManager::GetRandomizedCards(), BuffCardManager::CurrentSelectedCard());
		DrawDeck(BuffCardManager::GetRandomizedCards());
	}
	//if (AEInputCheckTriggered(AEVK_P)) { // Remember to set textloading back to false for fadeonce flag.
	//	textLoading = false;
	//}
}
void BuffCardScreen::Exit() {
	// Free meshes
	if (cardMesh) {
		AEGfxMeshFree(cardMesh);
	}
	if (rectMesh) {
		AEGfxMeshFree(rectMesh);
	}
	// Free textures
	if (cardBackTex) {
		AEGfxTextureUnload(cardBackTex);
	}
	for (auto& tex : cardFrontTex)
	{
		if (tex)
		{
			AEGfxTextureUnload(tex);
			tex = nullptr;
		}
	}
	for (auto& tex : cardRarityTex)
	{
		if (tex)
		{
			AEGfxTextureUnload(tex);
			tex = nullptr;
		}
	}
	// Free fonts
	AEGfxDestroyFont(buffPromptFont);
	AEGfxDestroyFont(cardBuffFont);
}