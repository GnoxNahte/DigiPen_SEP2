#include "ParticleSystem.h"
#include "MeshGenerator.h"
#include "../Game/Camera.h"
#include <iostream>

ParticleSystem::ParticleSystem(int initialSize) : pool(initialSize)
{
	particleMesh = MeshGenerator::GetSquareMesh(1.f);
	spawnRate = 10000.f;
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
	spawnRate = AEInputCheckCurr(AEVK_F) ? 10000.f : 100.f;

	float currTime = (float)AEGetTime(nullptr);
	while (currTime > lastSpawnTime)
	{
		double diff = 1.f / spawnRate;
		lastSpawnTime = lastSpawnTime + diff;
		SpawnParticle({ AERandFloat() * 1.f + 4.f, AERandFloat() * 1.f + 4.f });
	}
	
	if (pool.GetSize() == 0)
	{
		std::cout << "=== Empty pool ===\n";
		return;
	}

	std::cout << pool.GetSize() << "\n";

	float dt = (float)AEFrameRateControllerGetFrameTime();
	// todo - make custom iterator inside object pool instead?
	for (int i = static_cast<int>(pool.GetSize()) - 1; i >= 0; --i)
	{
		pool.pool[i].Update(dt);
		//std::cout << i <<  " | lifetime: " << (pool.pool[i].lifetime - currTime) << "\n";
		if (currTime > pool.pool[i].lifetime)
			pool.Release(pool.pool[i]);
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

Particle& ParticleSystem::SpawnParticle(const AEVec2& position)
{
	Particle& p = pool.Get();
	p.position = position;
	p.lifetime = (float)AEGetTime(nullptr) + 2.f;


	// TMP - Shoots out to the top right
	p.velocity.x = (AERandFloat()) * 2.f;
	p.velocity.y = (AERandFloat()) * 2.f;
	AEVec2Normalize(&p.velocity, &p.velocity);
	constexpr float speed = 10.f;
	p.velocity.x *= speed;
	p.velocity.y *= speed;
	return p;
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
