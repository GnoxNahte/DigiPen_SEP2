#pragma once
#include "ObjectPool.h"
#include "AEEngine.h"

struct Particle : public ObjectPoolItem
{
	AEVec2 position;
	AEVec2 velocity;
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
		AEVec2 angleRange;
		AEVec2 speedRange;
		AEVec2 lifetimeRange;
	};

	ParticleSystem(int initialSize);
	~ParticleSystem();

	void Init();
	void Update();
	void Render();
	void Free();

	Particle& SpawnParticle(const AEVec2& position);

	/**
	 * @brief				Spawn a burst of particles
	 * @param position		Center position of burst
	 * @param angleRange	Range of angle for the initial velocity. Range: [0, 360] (inclusive)
	 * @param speedRange	Range of speed for the initial velocity. Range: Anything AEVec2 supports
	 * @param spawnCount	Number of particles to spawn
	 */
	void SpawnParticleBurst(const AEVec2& position, size_t spawnCount);

	EmitterSettings emitter;
private:
	ObjectPool<Particle> pool;
	AEGfxVertexList* particleMesh = nullptr;
	float spawnRate = 0.f;
	double lastSpawnTime = 0.f;
};
