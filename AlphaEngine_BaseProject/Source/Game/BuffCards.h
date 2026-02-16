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
	void Init();
	void Render();
	void Exit();
	inline static AEGfxTexture* GetCardTexture() { return cardTex; }
	inline static AEGfxVertexList* GetCardMesh() { return cardMesh; }
private:
	// Card texture and mesh
	inline static AEGfxTexture* cardTex;
	inline static AEGfxVertexList* cardMesh;
	// Card sprite dimensions
	static const int CARD_WIDTH = 750;
	static const int CARD_HEIGHT = 1050;
};