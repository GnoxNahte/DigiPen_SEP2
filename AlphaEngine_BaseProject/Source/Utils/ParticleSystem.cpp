#include <iostream>
#include <limits>

#include "ParticleSystem.h"
#include "MeshGenerator.h"
#include "../Game/Camera.h"
#include "../Utils/AEExtras.h"
#include "../Game/Time.h"

ParticleSystem::ParticleSystem(int initialSize, const EmitterSettings& emitter) : 
	pool(initialSize),
	emitter(emitter)
{
	particleMesh = MeshGenerator::GetSquareMesh(1.f);
	//SetSpawnRate(10000.f);
}

ParticleSystem::~ParticleSystem()
{
	AEGfxMeshFree(particleMesh);
}

void ParticleSystem::Init()
{
	lastSpawnTime = static_cast<float>(Time::GetInstance().GetScaledElapsedTime());
}

void ParticleSystem::Update()
{
	float currTime = static_cast<float>(Time::GetInstance().GetScaledElapsedTime());
	while (currTime > lastSpawnTime)
	{
		lastSpawnTime += timeBetweenSpawn;
		Particle& p = SpawnParticle();
		p.Update(static_cast<float>(lastSpawnTime - currTime)); // Prebake
	}
	
	if (pool.GetSize() == 0)
		return;

	//std::cout << pool.GetSize() << " | " << timeBetweenSpawn << "\n";

	float dt = static_cast<float>(Time::GetInstance().GetScaledDeltaTime());
	// todo - make custom iterator inside object pool instead?
	for (int i = static_cast<int>(pool.GetSize()) - 1; i >= 0; --i)
	{
		Particle& p = pool.pool[i];
		p.Update(dt);
		if (currTime > p.spawnTime + p.lifetime)
			pool.Release(p);
	}
}

void ParticleSystem::Render()
{
	AEGfxTextureSet(nullptr, 0, 0);
	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	// todo - make custom iterator inside object pool instead?
	for (size_t i = 0; i < pool.GetSize(); i++)
		pool.pool[i].Render(particleMesh);

	AEGfxSetTransparency(1.f);
	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
}

void ParticleSystem::Free()
{
}

Particle& ParticleSystem::SpawnParticle()
{
	return SpawnParticle(emitter);
}

Particle& ParticleSystem::SpawnParticle(const EmitterSettings& _emitter)
{
	Particle& p = pool.Get();
	p.spawnTime = static_cast<float>(Time::GetInstance().GetScaledElapsedTime());
	p.position.x = AEExtras::RandomRange(_emitter.spawnPosRangeX);
	p.position.y = AEExtras::RandomRange(_emitter.spawnPosRangeY);
	p.lifetime =  AEExtras::RandomRange(_emitter.lifetimeRange);

	AEVec2FromAngle(&p.velocity, AEExtras::RandomRange(_emitter.angleRange));
	AEVec2Scale(&p.velocity, &p.velocity, AEExtras::RandomRange(_emitter.speedRange));

	return p;
}

void ParticleSystem::SpawnParticleBurst(const EmitterSettings& _emitter, size_t spawnCount)
{
	for (size_t i = 0; i < spawnCount; i++)
		SpawnParticle(_emitter);
}

void ParticleSystem::SpawnParticleBurst(size_t spawnCount)
{
	SpawnParticleBurst(emitter, spawnCount);
}

void ParticleSystem::SetSpawnRate(float spawnRate)
{
	if (spawnRate <= 0.f)
	{
		// Prevent spawning anything
		timeBetweenSpawn = (std::numeric_limits<float>::max)();
		lastSpawnTime = (std::numeric_limits<double>::max)();
		return;
	}

	timeBetweenSpawn = 1.f / spawnRate;
	float currTime = static_cast<float>(Time::GetInstance().GetScaledElapsedTime());
	if (lastSpawnTime - currTime > timeBetweenSpawn)
		lastSpawnTime = currTime + timeBetweenSpawn;
}

void Particle::Update(float dt)
{
	position.x += velocity.x * dt;
	position.y += velocity.y * dt;
}

void Particle::Render(AEGfxVertexList* mesh)
{
	AEMtx33 transform;

	AEMtx33Scale(&transform, 0.1f, 0.1f);
	AEMtx33TransApply(
		&transform,
		&transform,
		position.x - 0.5f,
		position.y - 0.5f
	);
	// Camera scale. Scales translation too.
	AEMtx33ScaleApply(&transform, &transform, Camera::scale, Camera::scale);
	AEGfxSetTransform(transform.m);
	AEGfxSetTransparency(1.f - (static_cast<float>(Time::GetInstance().GetScaledElapsedTime()) - spawnTime) / lifetime);
	AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
}

void Particle::Init()
{
}

void Particle::OnGet()
{
}

void Particle::OnRelease()
{
}

void Particle::Exit()
{
}
