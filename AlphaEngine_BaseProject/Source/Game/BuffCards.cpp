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


void BuffCardManager::Update() {
	if (!shuffled && AEInputCheckTriggered(AEVK_P)) {
		//currentCards = BuffCardManager::RandomizeCards(BuffCardScreen::NUM_CARDS);
		//SaveCardInfo();
		LoadCardInfo();
		//shuffled = true;
	}
}


/*-------------------------------------------------------------
Randomize card types and rarities for the current set of cards.
Called once per shuffle.
--------------------------------------------------------------*/
std::vector<BuffCard> BuffCardManager::RandomizeCards(int numCards) {
	std::vector<BuffCard> randomizedCards{};
	for (int i = 0; i < numCards; ++i) {

		/*--------------------------------------------------------------------
		Handling of card rarity. Rarity determines the strength of the buffs
		and the pool of possible buffs.
		----------------------------------------------------------------------*/
		CARD_RARITY rarity{};
		f32 rarityRoll = AEExtras::RandomRange({ 0.f, 1.f }); // Get a random float between 0 and 1 to determine rarity.

		std::cout << "RARITY ROLLED : " << rarityRoll << std::endl;
		// 50% chance for UNCOMMON, 25% for RARE, 15% for EPIC, 10% for LEGENDARY
		if (rarityRoll < 0.5) {
			rarity = RARITY_UNCOMMON;
			std::cout << "UNCOMMON" << std::endl;
		}
		else if (rarityRoll < 0.75) {
			rarity = RARITY_RARE;
			std::cout << "RARE" << std::endl;
		}
		else if (rarityRoll < 0.9) {
			rarity = RARITY_EPIC;
			std::cout << "EPIC" << std::endl;
		}
		else {
			rarity = RARITY_LEGENDARY;
			std::cout << "LEGENDARY" << std::endl;
		}
		/*--------------------------------------------------------------------
		Handling of card type. Some card types only appear in certain rarities,
		such as legendary cards only having access to the most powerful buffs.
		----------------------------------------------------------------------*/
		f32 cardTypeRoll = AEExtras::RandomRange({ 0.f, 1.f });
		// Handling the type of the cards.
		CARD_TYPE type{};
		if (rarity == RARITY_LEGENDARY) { // If legendary, guarantee switch it up or revitalise.
			type = (cardTypeRoll < 0.5f) ? SWITCH_IT_UP : REVITALIZE;
		}
		else { // Otherwise, all types are available.
			if (cardTypeRoll < 0.2f) {
				type = HERMES_FAVOR;
			}
			else if (cardTypeRoll < 0.4f) {
				type = IRON_DEFENCE;
			}
			else if (cardTypeRoll < 0.6f) {
				type = SWITCH_IT_UP;
			}
			else if (cardTypeRoll < 0.8f) {
				type = REVITALIZE;
			}
			else {
				type = SHARPEN;
			}
		}

		// Create card name and description based on type and rarity (placeholder logic).
		std::string cardName = "Card " + std::to_string(i + 1);
		std::string cardDesc = "A mysterious buff.";
		randomizedCards.push_back(BuffCard(rarity, type, cardName, cardDesc));
	}
	for (int i = 0; i < numCards; ++i) {
		std::cout << randomizedCards[i].cardName << " - Rarity: " << BuffCardManager::CardRarityToString(randomizedCards[i].rarity)
			<< ", Type: " << BuffCardManager::CardTypeToString(randomizedCards[i].type) << std::endl;
	}
	return randomizedCards;
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

void BuffCardManager::SaveCardInfo() {
	rapidjson::Document doc;
	doc.SetObject();
	auto& allocator = doc.GetAllocator();

	// Create a JSON array to store all cards
	rapidjson::Value cardArray(rapidjson::kArrayType);

	for (auto& card : currentCards) {
		rapidjson::Value cardObj(rapidjson::kObjectType);

		cardObj.AddMember("type",
			rapidjson::Value(BuffCardManager::CardTypeToString(card.type).c_str(), allocator),
			allocator);

		cardObj.AddMember("rarity",
			rapidjson::Value(BuffCardManager::CardRarityToString(card.rarity).c_str(), allocator),
			allocator);

		// Save name and description as strings
		cardObj.AddMember("name", rapidjson::Value(card.cardName.c_str(), allocator), allocator);
		cardObj.AddMember("description", rapidjson::Value(card.cardDesc.c_str(), allocator), allocator);

		// Add this card to the array
		cardArray.PushBack(cardObj, allocator);
	}

	// Add the array to the root document
	doc.AddMember("cards", cardArray, allocator);

	// Build the path
	std::string actualAssetPath = "../../" + file;

	// Ensure the directory exists before writing
	std::filesystem::create_directories(std::filesystem::path(actualAssetPath).parent_path());

	// Save JSON
	FileHelper::TryWriteJsonFile(actualAssetPath, doc);
}

void BuffCardManager::LoadCardInfo() {
	currentCards.clear(); // clear previous cards

	// Build the path to the JSON file
	std::string actualAssetPath = "../../" + file;

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
		if (cardObj.HasMember("type") && cardObj["type"].IsString())
			card.type = GetCardType(cardObj["type"].GetString());

		if (cardObj.HasMember("rarity") && cardObj["rarity"].IsString())
			card.rarity = GetCardRarity(cardObj["rarity"].GetString());

		// Read name and description
		if (cardObj.HasMember("name") && cardObj["name"].IsString())
			card.cardName = cardObj["name"].GetString();

		if (cardObj.HasMember("description") && cardObj["description"].IsString())
			card.cardDesc = cardObj["description"].GetString();

		currentCards.push_back(card);

		std::cout << "Loaded card: " << card.cardName << " - Rarity: " << CardRarityToString(card.rarity)
			<< ", Type: " << CardTypeToString(card.type) << std::endl;
	}

	std::cout << "Loaded " << currentCards.size() << " cards from JSON." << std::endl;
}


BuffCard::BuffCard(CARD_RARITY cr, // Constructor
	CARD_TYPE ct,
	std::string cName,
	std::string cDesc,
	AEVec2 pos) :
	rarity{ cr }, type{ ct }, cardName{ cName }, cardDesc{ cDesc }, cardPos{ pos } {/* empty by design */}

void BuffCardScreen::Init() {
	rectMesh = MeshGenerator::GetRectMesh(1.0f, 1.0f);
	cardMesh = MeshGenerator::GetRectMesh(1.0f, 1.0f);
	cardBackTex = AEGfxTextureLoad("Assets/0_CardBack.png");
	cardFrontTex[HERMES_FAVOR] = AEGfxTextureLoad("Assets/Hermes_Favor.png");
	cardFrontTex[IRON_DEFENCE] = AEGfxTextureLoad("Assets/Iron_Defence.png");
	cardFrontTex[SWITCH_IT_UP] = AEGfxTextureLoad("Assets/Switch_It_Up.png");
	buffPromptFont = AEGfxCreateFont("Assets/m04.ttf", BUFF_PROMPT_FONT_SIZE);
	srand(static_cast<unsigned int>(time(NULL))); // Seed random number generator with current time for variability.
}

void BuffCardScreen::Update() {

	//const float FLIP_SPEED = 5.0f; // Adjust flip speed

	//for (int i = 0; i < NUM_CARDS; ++i) {
	//	if (cardFlipping[i]) {
	//		// Animate from -1.0 (back) to 1.0 (front)
	//		cardFlipStates[i] += static_cast<f32>(FLIP_SPEED * AEFrameRateControllerGetFrameTime())	;

	//		if (cardFlipStates[i] >= 1.0f) {
	//			cardFlipStates[i] = 1.0f;
	//			cardFlipping[i] = false; // Flip complete
	//		}
	//	}
	//}
	//// Automated sequential flipping
	//if (!allCardsFlipped) {
	//	// Check if we need to start flipping the next card
	//	if (currentFlipIndex < NUM_CARDS) {
	//		// Create timer for this card if not created yet
	//		std::string timerName = "Flip Timer " + std::to_string(currentFlipIndex);

	//		if (!flipTimerCreated[currentFlipIndex]) {
	//			float delay = 0.45f; // Delay between cards
	//			TimerSystem::GetInstance().AddTimer(timerName, delay, false);
	//			flipTimerCreated[currentFlipIndex] = true;
	//		}

	//		auto* timer = TimerSystem::GetInstance().GetTimerByName(timerName);
	//		// Check if timer completed
	//		if (timer &&
	//			timer->completed) {
	//			FlipCard(currentFlipIndex);
	//			TimerSystem::GetInstance().RemoveTimer(timerName);
	//			currentFlipIndex++; // Move to next card
	//		}
	//	}
	//	else {
	//		allCardsFlipped = true; // All cards done
	//	}
	//}
	//if (AEInputCheckTriggered(AEVK_P)) { // Remember to set textloading back to false for fadeonce flag.
	//	ResetFlipSequence();
	//}
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