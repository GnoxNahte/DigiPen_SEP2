#pragma once
#include "ObjectPool.h"
#include "AEEngine.h"

struct Particle : public ObjectPoolItem
{
	AEVec2 position;
	AEVec2 velocity;
	float spawnTime;
	float lifetime;

	virtual void Update(float dt);
	virtual void Render(AEGfxVertexList* mesh);

	virtual void Init() override;
	virtual void OnGet() override;
	virtual void OnRelease() override;
	virtual void Exit() override;
};

class ParticleSystem
{
public:
	struct EmitterSettings
	{
		// Box range of where the particles can spawn
		// Maybe can support other shapes like circle
		AEVec2 spawnPosRangeX;
		AEVec2 spawnPosRangeY;

		// Range of angle for the initial velocity. Range: [0, 360] (inclusive)
		AEVec2 angleRange;
		// Range of speed for the initial velocity.
		AEVec2 speedRange;
		// Range of lifetime.
		AEVec2 lifetimeRange;
	};

	ParticleSystem(int initialSize);
	~ParticleSystem();

	void Init();
	void Update();
	void Render();
	void Free();

	Particle& SpawnParticle();

	/**
	 * @brief				Spawn a burst of particles
	 * @param position		Center position of burst
	 * @param spawnCount	Number of particles to spawn
	 */
	void SpawnParticleBurst(const AEVec2& position, size_t spawnCount);

	void SetSpawnRate(float spawnRate);

	EmitterSettings emitter;
private:
	float timeBetweenSpawn = 0.f;
	ObjectPool<Particle> pool;
	AEGfxVertexList* particleMesh = nullptr;
	double lastSpawnTime = 0.f;
};
