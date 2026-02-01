#pragma once

#include <vector>
#include <functional>

#include "EditorUtils.h"

class Editor
{
public:
	static void Register(Inspectable& obj);
	static void Unregister(Inspectable& obj);

	static void Update();
	static void DrawInspectors();

private:
	// Singleton
	static Editor& Get();

	Editor();

	bool showInspectors = false;
	bool showDemoWindow = false;
	bool showColliders = false;

	// @todo - benchmark, change to map if got lots of unregisters
	std::vector<std::reference_wrapper<Inspectable>> menuObjs;
};

