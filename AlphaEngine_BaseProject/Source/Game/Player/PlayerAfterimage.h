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
		AEVec2 position {};
		float spawnTime = -1.f;
		bool isFacingRight;
		bool isDashing;

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
	AEVec2 dash_uvOffset;
	AEVec2 slam_uvOffset;
	float dashSpacing;
	float slamSpacing;
};

