#pragma once
#include "AEEngine.h"

// Enumeration types for the card rarity.
enum CARD_RARITY {
	RARITY_UNCOMMON,
	RARITY_RARE,
	RARITY_EPIC,
	RARITY_LEGENDARY
};

class BuffCards {
public:
	static void Init();
	static void Render();
	static void Exit();
	inline AEGfxTexture* GetCardBackTexture() const { return cardBackTex; }
	inline AEGfxVertexList* GetCardMesh() const { return cardMesh; }
private:
	// Card texture and mesh
	inline static AEGfxTexture* cardBackTex = nullptr;
	inline static AEGfxVertexList* cardMesh = nullptr;

	// Card sprite dimensions
	static const int CARD_WIDTH = 750;
	static const int CARD_HEIGHT = 1050;
};