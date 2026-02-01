#include "GameObject.h"

void Component::Awake() {}
void Component::OnEnable() {}
void Component::Start() {}
void Component::Update() {}
void Component::Render() {}
void Component::OnDisable() {}
void Component::OnDestroy() {}

void Component::DrawInspector(){}

void GameObjectManager::Init()
{
}

void GameObjectManager::Update()
{
}

void GameObjectManager::Render()
{
}

void GameObjectManager::Exit()
{
}

GameObjectManager& GameObjectManager::Get()
{
	static GameObjectManager manager;
	return manager;
}

GameObjectManager::GameObjectManager() 
{
	gameObjects.reserve(50);
}

void GameObject::Update()
{
	/*for (auto& i : components)
	{
		i.
	}*/
}

void GameObject::Render()
{
}
