#include "ParticleSystem.h"
#include "MeshGenerator.h"
#include "../Game/Camera.h"
#include <iostream>

static float testNextSpawnTime = -1.f;

ParticleSystem::ParticleSystem(int initialSize) 
	:	pool(initialSize), 
		particleMesh(nullptr)
{
	particleMesh = MeshGenerator::GetSquareMesh(1.f);
}

void ParticleSystem::Init()
{
	testNextSpawnTime = 3.f;
}

void ParticleSystem::Update()
{
	float currTime = (float)AEGetTime(nullptr);
	if (currTime > testNextSpawnTime)
	{
		SpawnParticle({ AERandFloat() * 2.f + 4.f, AERandFloat() * 2.f + 4.f });
		SpawnParticle({ AERandFloat() * 2.f + 4.f, AERandFloat() * 2.f + 4.f });
		SpawnParticle({ AERandFloat() * 2.f + 4.f, AERandFloat() * 2.f + 4.f });
		SpawnParticle({ AERandFloat() * 2.f + 4.f, AERandFloat() * 2.f + 4.f });
		SpawnParticle({ AERandFloat() * 2.f + 4.f, AERandFloat() * 2.f + 4.f });
		testNextSpawnTime = currTime + 0.001f;
	}
	
	if (pool.GetSize() == 0)
	{
		std::cout << "=== Empty pool ===\n";
		return;
	}

	//pool.DebugPrint();

	// todo - make custom iterator inside object pool instead?
	for (int i = static_cast<int>(pool.GetSize()) - 1; i >= 0; --i)
	{
		pool.pool[i].Update(1.f);
		//std::cout << i <<  " | lifetime: " << (pool.pool[i].lifetime - currTime) << "\n";
		if (currTime > pool.pool[i].lifetime)
			pool.Release(pool.pool[i]);
	}

	//pool.DebugPrint();
	//std::cout << "===============================" << "\n";
}

void ParticleSystem::Render()
{
	AEGfxTextureSet(nullptr, 0, 0);
	// todo - make custom iterator inside object pool instead?
	for (size_t i = 0; i < pool.GetSize(); i++)
	{
		pool.pool[i].Render(particleMesh);
	}
}

void ParticleSystem::Free()
{
}

Particle& ParticleSystem::SpawnParticle(const AEVec2& position)
{
	Particle& p = pool.Get();
	p.position = position;
	p.lifetime = (float)AEGetTime(nullptr) + 5.f;

	float speed = 0.05f;
	p.velocity.x = (AERandFloat() - 0.5f) * 2.f * speed;
	p.velocity.y = (AERandFloat() - 0.5f) * 2.f * speed;
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

	AEGfxSetRenderMode(AE_GFX_RM_COLOR);

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
	
	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
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
