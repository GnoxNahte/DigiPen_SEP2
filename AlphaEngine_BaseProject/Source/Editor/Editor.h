#pragma once

#include <vector>
#include <map>

#include "EditorUtils.h"
#include "../Game/Scene/GSM.h"
#include "../Utils/Event/EventSystem.h"

class Editor
{
public:
	struct EditorPrefs
	{
		SceneState lastOpenedScene = SceneState::GS_GAME;
	};

	// Inspectable automatically registers and unregisters in constructor
	static void Register(Inspectable* obj);
	static void Unregister(Inspectable* obj);

	// Each system needs to register by themselves because need the name. 
	// @todo change inspectable to hold a name too then can combine Register and RegisterSystem
	static void RegisterSystem(std::string name, Inspectable* obj);
	static void UnregisterSystem(std::string name, Inspectable* obj);

	static const EditorPrefs& GetEditorPrefs();

	static void Update();
	static void DrawInspectors();

	static bool GetShowColliders();
private:

	inline static std::string editorPrefsPath = "Assets/Editor/prefs.json";
	EditorPrefs editorPrefs;

	bool showInspectors = false;
	bool showDemoWindow = false;
	bool showColliders = false;

	Inspectable* focusedObject = nullptr;

	EventId gsmSceneChangeEventId;

	// @todo - benchmark, change to map if got lots of unregisters
	std::vector<Inspectable*> gameObjs;
	std::map<std::string, Inspectable*> systemsObjs;

	// Singleton
	static Editor& Get();
	Editor();
	~Editor();

	void DrawMenus();
	void LoadEditorPrefs();
	void SaveEditorPrefs();

	void OnSceneChange(const SceneChangeEvent& ev);
};
