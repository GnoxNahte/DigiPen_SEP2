/*!
@file	PlayerAfterimage.h
@author	Ethan Ong
@brief	Declares PlayerAfterimage, a system for spawning, updating 
		and rendering the player afterimages

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
#pragma once

#include <AEVec2.h>
#include <AEGraphics.h>
#include "../../Utils/ObjectPool.h"

class Player; // Forward declare since Player also includes this file

class PlayerAfterimage
{
public:
	PlayerAfterimage(const Player& player, const AEVec2& size);
	~PlayerAfterimage();

	void Render();
	void Update();

	void ResetLastSpawn();

private:
	struct Afterimage : public ObjectPoolItem
	{
		enum Type
		{
			DASH_UP,
			DASH_DOWN,
			DASH_HORIZONTAL,
			SMASH_ATTACK,

			TOTAL_TYPE
		};
		AEVec2 position{};
		float spawnTime = -1.f;
		bool isFacingRight = false;
		Type type = DASH_HORIZONTAL;

		void Init() override;
		void OnGet() override;
		void OnRelease() override;
		void Exit() override;
	};

	// Using a object pool is probably overkill but just using cos I'm familiar with it
	// maybe use a queue insted since it always spawns and despawn in the same order
	ObjectPool<Afterimage> pool;
	AEGfxTexture* texture;
	AEGfxVertexList* mesh;

	const Player& refPlayer;

	AEVec2 lastSpawnedPos;

	// === Settings ===
	float lifetime;
	AEVec2 uvOffsets[Afterimage::TOTAL_TYPE];
	float spacings[Afterimage::TOTAL_TYPE];
};

