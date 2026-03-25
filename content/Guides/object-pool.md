# Object Pool usage {#object_pool_usage}
## Description
Object Pool for quickly spawning and releasing items.

@warning This will swap memory around when releasing. So note that pointers stored to the object item might point to different data. This also means it's not ordered.<br><br>So, this is best for stuff like ParticleSystem where there's no need to reference any particle directly

Features:
- Time complexity:
	- Get - O(1)
	* Release - O(1)
* Elements are continuous in memory so very efficient to loop through (For example when updating/rendering)

## Usage
### Item {#object-pool-item}
1. Create a class and inherit ObjectPoolItem
2. Implement `override` functions 
	- `Init` - Called when creating the item
	- `OnGet` - Called when getting/enabling it
	- `OnRelease` - Called when releasing/disabling it
	- `Exit` - Called when deleting the item
	- Syntax: `virutal void FunctionName() override;`
3. Can add other variables/functions if needed

Reference: Particle
### Pool
#### Setup
Make a variable in the class to store the ObjectPool
- Syntax: `ObjectPool<ItemName>` 
- The item class should be the same one as the one u created above - @ref object-pool-item
- Item class should inherit ObjectPoolItem. If not will have syntax error

#### Using
- Call ObjectPool.Get to get an item from pool
- When finished using, call ObjectPool.Release

Looping through the items is abit messy for now... might make a custom iterator.<br>But for now, 
- Use ObjectPool.GetSize() and access the pool vector index
- But if u are releasing items while looping, need to loop from the back

An example from the particle system:
```cpp
for (int i = static_cast<int>(pool.GetSize()) - 1; i >= 0; --i)
{
	Particle& p = pool.pool[i];
	p.Update(dt);
	if (currTime > p.spawnTime + p.lifetime)
		pool.Release(p);
}
```

@note ObjectPool doesn't update/render anything. It just stores the items. Need to update/render yourself

Reference: ParticleSystem

