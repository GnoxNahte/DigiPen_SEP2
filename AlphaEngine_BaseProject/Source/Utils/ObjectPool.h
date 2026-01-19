#pragma once

#include <vector>
#include <iostream>

/**
 * @brief Abstract Object Pool class
 */
class ObjectPoolItem
{
public:
	size_t poolIndex;
	bool isActive;

	ObjectPoolItem() = default;
	ObjectPoolItem(ObjectPoolItem&&) = default;
	ObjectPoolItem& operator=(ObjectPoolItem&&) = default;

	// On instantiating pool
	virtual void Init() = 0;

	// On get from pool
	virtual void OnGet() = 0;
	// Called when release from pool (Set object to inactive but still in memory)
	virtual void OnRelease() = 0;

	virtual void Exit() = 0;
};

/**
 * @brief	Object Pool with time complexity of:
 *			- Get:		O(1) (Assuming no need to expand vector)
 *			- Release:	O(1)
 * 
 * 
 * @warning	This will swap memory around when removing so shouldn't store pointers to the items and it's not ordered
 */
template<typename T>
requires std::derived_from<T, ObjectPoolItem>
class ObjectPool
{
public:
	ObjectPool(int startSize);
	~ObjectPool();

	T& Get();
	void Release(T& item);

	size_t GetSize();

	void DebugPrint();

	// tmp - make private
	std::vector<T> pool;
private:
	size_t size; 
};

template<typename T>
requires std::derived_from<T, ObjectPoolItem>
inline ObjectPool<T>::ObjectPool(int startSize) : pool(startSize)
{
	for (size_t i = 0; i < pool.size(); i++)
	{
		ObjectPoolItem& item = pool[i];
		item.Init();
		item.poolIndex = static_cast<int>(i);
		item.isActive = false;
	}

	size = 0;
}

template<typename T>
requires std::derived_from<T, ObjectPoolItem>
inline ObjectPool<T>::~ObjectPool()
{
}

template<typename T>
requires std::derived_from<T, ObjectPoolItem>
inline T& ObjectPool<T>::Get()
{
	if (size == pool.size())
	{
		auto& item = pool.emplace_back();
		item.isActive = true;
		item.poolIndex = size++;
		return item;
	}
	else
	{
		auto& item = pool[static_cast<int>(size++)];
		item.isActive = true;
		return item;
	}
}

template<typename T>
requires std::derived_from<T, ObjectPoolItem>
inline void ObjectPool<T>::Release(T& item)
{
	--size;
	item.isActive = false;
	item.OnRelease();

	// If it's the last item, just return as don't need to swap
	/*if (item.poolIndex == size - 1)
		return;
	*/

	size_t itemIndex = item.poolIndex;
	std::swap(pool[itemIndex], pool[size]);
	std::swap(pool[itemIndex].poolIndex, pool[size].poolIndex);
}

template<typename T>
requires std::derived_from<T, ObjectPoolItem>
inline size_t ObjectPool<T>::GetSize()
{
	return size;
}

template<typename T>
	requires std::derived_from<T, ObjectPoolItem>
inline void ObjectPool<T>::DebugPrint()
{
	std::cout << "=== Object Pool ===\n";

	for (auto& i : pool)
	{
		/*Particle* p = dynamic_cast<Particle*>(&i);
		if (p) 
			std::cout << i.isActive << " - " << (p->lifetime - AEGetTime(nullptr)) << "\n";
		else*/
			std::cout << i.poolIndex << " - " << i.isActive << "\n";
	}

	std::cout << "===================\n";
}
