#pragma once

#include "CommonTypes.h"
#include <unordered_map>
#include <concepts>

// Forward declaration
class GameObject;

// Similar to Unity's Monobehaviour
class Component
{
public:
	// Execution order following Unity - https://docs.unity3d.com/6000.3/Documentation/Manual/execution-order.html
	virtual void Awake();
	virtual void OnEnable();
	virtual void Start();
	virtual void Update();
	virtual void Render();
	virtual void OnDisable();
	virtual void OnDestroy();

	virtual void DrawInspector();

	//// === Getters ===
	//GameObject* GetGameObject();
	//
	//bool IsEnabled();
private:
	GameObject* gameObject;
	bool isEnabled; 
};

class GameObject
{
public:
	template<typename T>
	requires std::derived_from<T, Component>
	T GetComponent();

	void Update();
	void Render();

	bool enableInspector = false;
	std::vector<Component> component;

private:
};

class GameObjectManager
{
public:
	static GameObject Instantiate();

	// === Only called in scenes ===
	static void Init();
	static void Update();
	static void Render();
	static void Exit();

private:
	// Singleton
	static GameObjectManager& Get();
	GameObjectManager();

	std::vector<GameObject> gameObjects;
};

template<typename T>
requires std::derived_from<T, Component>
inline T GameObject::GetComponent()
{
	return T();
}
