/*!
@file	Sprite.h
@author	Ethan Ong
@brief	Declares Sprite class which handles sprite animation, linked with sprite metadata\n
		Docs: https://gnoxnahte.github.io/DigiPen_SEP2/sprite_usage.html

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
#pragma once

#include "AEEngine.h"
#include <string>
#include <functional>
#include "SpriteMetadata.h"
#include <AEVec2.h>

class Sprite
{
public:
	Sprite(const char* file);
	~Sprite();

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

	int GetState() const;
	void SetState(int nextState, bool ifLock = false, std::function<void(int)> _onAnimEnd = {});
	
	const char* GetFile() const;
	AEVec2 GetUV_Size() const;

	const SpriteMetadata metadata;
private:

	// === Data derived from metadata ===
	float uvWidth, uvHeight;

	// === Runtime data that will change ===
	float animTimer;
	/**
	 * @brief Current state the animation is in. Row index in spritesheet
	 */
	int currStateIndex;
	/**
	 * @brief Next state the animation is in. Row index in spritesheet
	 */
	int nextStateIndex;

	/**
	 * @brief Current frame the animation is in. Column index in spritesheet
	 */
	int frameIndex;

	/**
	 * @brief Lock current frame until it finishes
	 */
	bool ifLockCurrent;
	
	AEVec2 uvOffset; // Current uv offset

	std::function<void(int)> onAnimEnd;
	const char* file;

	// === Mesh data ===
	AEGfxVertexList* mesh;
	AEGfxTexture* texture;
};

