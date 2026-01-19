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
	ParticleSystem(int initialSize);

	void Init();
	void Update();
	void Render();
	void Free();

	Particle& SpawnParticle(const AEVec2& position);
private:
	ObjectPool<Particle> pool;
	AEGfxVertexList* particleMesh;
};
