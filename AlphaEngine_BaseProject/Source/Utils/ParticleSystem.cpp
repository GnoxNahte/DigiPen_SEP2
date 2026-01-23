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
	SetSpawnRate(10000.f);
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
	SetSpawnRate(AEInputCheckCurr(AEVK_F) ? 10000.f : 0.f);

	if (AEInputCheckTriggered(AEVK_G))
		SpawnParticleBurst({ 2,2 }, 300);

	float currTime = (float)AEGetTime(nullptr);
	while (currTime > lastSpawnTime)
	{
		lastSpawnTime += timeBetweenSpawn;
		SpawnParticle();
		// todo? - prebake
	}
	
	if (pool.GetSize() == 0)
	{
		//std::cout << "=== Empty pool ===\n";
		return;
	}

	std::cout << pool.GetSize() << " | " << timeBetweenSpawn << "\n";

	float dt = (float)AEFrameRateControllerGetFrameTime();
	// todo - make custom iterator inside object pool instead?
	for (int i = static_cast<int>(pool.GetSize()) - 1; i >= 0; --i)
	{
		Particle& p = pool.pool[i];
		p.Update(dt);
		//std::cout << i <<  " | lifetime: " << (pool.pool[i].lifetime - currTime) << "\n";
		if (currTime > p.spawnTime + p.lifetime)
			pool.Release(p);
	}

	//pool.DebugPrint();
	//std::cout << "===============================" << "\n";
}

void ParticleSystem::Render()
{
	//return;
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

	//// TMP - Shoots out to the top right
	//p.velocity.x = (AERandFloat()) * 2.f;
	//p.velocity.y = (AERandFloat()) * 2.f;
	//AEVec2Normalize(&p.velocity, &p.velocity);
	//constexpr float speed = 10.f;
	//p.velocity.x *= speed;
	//p.velocity.y *= speed;
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
		timeBetweenSpawn = (std::numeric_limits<float>::max)();
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
