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
			// iterate from back. Use this weird syntax because size_t is unsigned
			for (size_t i = pool.GetSize(); (i--) > 0;)
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

void ParticleSystem::ReleaseAll()
{
	// iterate from back. Use this weird syntax because size_t is unsigned
	for (size_t i = pool.GetSize(); (i--) > 0;)
		pool.Release(pool.pool[i]);
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
	p.lifetime = AEExtras::RandomRange(_emitter.lifetimeRange);
	p.behavior = _emitter.behavior;
	p.behaviorParams = _emitter.behaviorParams;

	AEVec2FromAngle(&p.velocity, AEExtras::RandomRange(_emitter.angleRange));
	AEVec2Scale(&p.velocity, &p.velocity, AEExtras::RandomRange(_emitter.speedRange));

	p.tint = _emitter.tint;
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
	switch (behavior)
	{
	case ParticleBehavior::normal:
		position.x += velocity.x * dt;
		position.y += velocity.y * dt;
		break;

	case ParticleBehavior::Inward:
	{
		AEVec2 dir{ behaviorParams.center.x - position.x,
					behaviorParams.center.y - position.y };
		float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
		if (len > 0.0001f) { dir.x /= len; dir.y /= len; }

		velocity.x += dir.x * behaviorParams.pull * dt;
		velocity.y += dir.y * behaviorParams.pull * dt;

		position.x += velocity.x * dt;
		position.y += velocity.y * dt;
		break;
	}

	case ParticleBehavior::TornadoIn:
	{
		// swirl tangent around center + inward pull
		AEVec2 toC{ behaviorParams.center.x - position.x,
					behaviorParams.center.y - position.y };
		float len = sqrtf(toC.x * toC.x + toC.y * toC.y);
		if (len > 0.0001f) { toC.x /= len; toC.y /= len; }

		// tangent (perpendicular)
		AEVec2 tan{ -toC.y, toC.x };

		velocity.x += (toC.x * behaviorParams.pull + tan.x * behaviorParams.swirl) * dt;
		velocity.y += (toC.y * behaviorParams.pull + tan.y * behaviorParams.swirl) * dt;

		position.x += velocity.x * dt;
		position.y += velocity.y * dt;
		break;
	}

	case ParticleBehavior::Gravity:
		velocity.y -= behaviorParams.pull * dt; // reuse pull as gravity
		position.x += velocity.x * dt;
		position.y += velocity.y * dt;
		break;
	}
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

	const float age = static_cast<float>(Time::GetInstance().GetScaledElapsedTime()) - spawnTime;
	const float t = (lifetime > 0.f) ? (age / lifetime) : 1.f;
	const float fade = 1.f - AEClamp(t, 0.f, 1.f);

	AEGfxSetColorToMultiply(tint.r, tint.g, tint.b, 1.f);
	AEGfxSetTransparency(tint.a * fade);

	//AEGfxSetTransparency(1.f - (static_cast<float>(Time::GetInstance().GetScaledElapsedTime()) - spawnTime) / lifetime);
	AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);

	AEGfxSetColorToMultiply(1.f, 1.f, 1.f, 1.f);
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
