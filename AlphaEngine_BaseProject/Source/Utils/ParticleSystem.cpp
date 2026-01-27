#include "ParticleSystem.h"
#include "MeshGenerator.h"
#include "../Game/Camera.h"
#include "../Utils/AEExtras.h"
#include <iostream>
#include <limits>

ParticleSystem::ParticleSystem(int initialSize) : pool(initialSize),
	// Test only
	emitter{
		{0, 0}, // spawn pos min
		{0, 0}, // spawn pos max
		{AEDegToRad(20.f), AEDegToRad(40.f)}, // angle range (20 deg - 40 deg)
		{15.f, 30.f}, // speed range
		{ 1.5f, 3.f } // lifetime range
	}
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
	lastSpawnTime = (float)AEGetTime(nullptr);
}

void ParticleSystem::Update()
{
	float currTime = (float)AEGetTime(nullptr);
	while (currTime > lastSpawnTime)
	{
		lastSpawnTime += timeBetweenSpawn;
		Particle& p = SpawnParticle();
		p.Update(static_cast<float>(lastSpawnTime - currTime)); // Prebake
	}
	
	if (pool.GetSize() == 0)
		return;

	//std::cout << pool.GetSize() << " | " << timeBetweenSpawn << "\n";

	float dt = (float)AEFrameRateControllerGetFrameTime();
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

	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
}

void ParticleSystem::Free()
{
}

Particle& ParticleSystem::SpawnParticle()
{
	Particle& p = pool.Get();
	p.spawnTime = (float)AEGetTime(nullptr);
	p.position.x = AEExtras::RandomRange(emitter.spawnPosRangeX);
	p.position.y = AEExtras::RandomRange(emitter.spawnPosRangeY);
	p.lifetime =  AEExtras::RandomRange(emitter.lifetimeRange);

	AEVec2FromAngle(&p.velocity, AEExtras::RandomRange(emitter.angleRange));
	AEVec2Scale(&p.velocity, &p.velocity, AEExtras::RandomRange(emitter.speedRange));

	return p;
}

void ParticleSystem::SpawnParticleBurst(const AEVec2& position, size_t spawnCount)
{
	float currTime = (float)AEGetTime(nullptr);

	for (size_t i = 0; i < spawnCount; i++)
	{
		Particle& p = pool.Get();
		p.spawnTime = currTime;
		p.position = position;
		p.lifetime = AEExtras::RandomRange(emitter.lifetimeRange);

		AEVec2FromAngle(&p.velocity, AEExtras::RandomRange(emitter.angleRange));
		AEVec2Scale(&p.velocity, &p.velocity, AEExtras::RandomRange(emitter.speedRange));
	}
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
	float currTime = (float)AEGetTime(nullptr);
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
	AEGfxSetTransparency(1.f - ((float)AEGetTime(nullptr) - spawnTime) / lifetime);
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
