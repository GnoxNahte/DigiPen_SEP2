## Particle System {#particle_system}

## Description
Uses \ref object_pool_usage "Object Pool", read the Description heading as everything that's in there applies to this too.

So main thing is shouldn't reference any particle directly (through references or pointers) as it's data might be swapped with another particle at any time

## Setup
- Add the ParticleSystem class in ur class,
- In constructor, set the particle emitter settings. This is used for spawning in update. Not for burst
- Call ParticleSystem.Update() and ParticleSystem.Render()

When constructing the emitter settings, can use this format to make it readable. Syntax: [cppreference - Aggregate initialization](https://en.cppreference.com/w/cpp/language/aggregate_initialization.html) 
```cpp
ParticleSystem::EmitterSettings{ 
	.angleRange{ PI / 3, PI / 4 },
	.speedRange{ 30.f, 50.f },
	.lifetimeRange{1.f, 2.f},
} 
```

## Burst 
Call ParticleSystem.SpawnParticleBurst. Set spawn count, has an overload pass in a override the default emitter if u want.
```cpp
particleSystem.SpawnParticleBurst(500);

// Overriding the emitter 
// Try not to keep creating this if u do this often. Create it somewhere then reference it later
ParticleSystem::EmitterSettings emitSettings {
	.angleRange{ PI / 3, PI / 4 },
	.speedRange{ 30.f, 50.f },
	.lifetimeRange{1.f, 2.f},
}
particleSystem.SpawnParticleBurst(emitSettings, 500)
```


