#pragma once

#include <vector>
#include <functional>

/**
 * @brief An interface for objects that can be inspected (Shows up in editor)
 * @todo make some parts debug only
 */
class Inspectable
{
public:
	bool enableInspector = false;
	Inspectable();
	~Inspectable();
	virtual void DrawInspector() = 0;
};

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

	// @todo - benchmark, change to map if got lots of unregisters
	std::vector<std::reference_wrapper<Inspectable>> menuObjs;
};

