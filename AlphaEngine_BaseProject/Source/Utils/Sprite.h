#pragma once

#include "AEEngine.h"
#include <string>
#include "SpriteMetadata.h"

class Sprite
{
public:
	Sprite(std::string file);

	/**
	 * @brief Update sprite animation
	 */
	void Update();

	/**
	 * @brief	Sets the Texture and Draw.
	 * @warning DOES NOT set the transform. Only sets the texture and Draw. 
	 *			Set the transform before calling this
	 */
	void Render();
private:
	SpriteMetadata metadata;

	// === Data derived from metadata ===
	float timePerFrame;
	float uvWidth, uvHeight;

	// === Runtime data that will change ===
	float animTimer;
	int spriteIndex;

	AEVec2 uvOffset; // Current uv offset

	// === Mesh data ===
	AEGfxVertexList* mesh;
	AEGfxTexture* texture;
};

