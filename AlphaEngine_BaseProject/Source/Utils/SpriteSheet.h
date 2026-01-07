#pragma once

#include "AEEngine.h"
#include <string>

class SpriteSheet
{
public:
	SpriteSheet(std::string file, int rows, int cols, float framesPerSecond);

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
	float timePerFrame;
	int rows, cols;
	float uvWidth, uvHeight;

	float animTimer;
	int spriteIndex;

	AEVec2 uvOffset; // Current uv offset

	AEGfxVertexList* mesh;
	AEGfxTexture* texture;
};

