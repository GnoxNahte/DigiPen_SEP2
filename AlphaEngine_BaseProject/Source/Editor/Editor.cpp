#include "Editor.h"
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <AEEngine.h>

#include "../Utils/AEExtras.h"
#include "../Utils/FileHelper.h"
#include "../Game/Time.h"

#undef GetObject

void Editor::Register(Inspectable* obj)
{
	Get().gameObjs.emplace_back(obj);
}

void Editor::Unregister(Inspectable* obj)
{
	auto& objs = Get().gameObjs;
	std::erase(objs, obj);
}

void Editor::RegisterSystem(std::string name, Inspectable* obj)
{
	Get().systemsObjs.try_emplace(name, obj);
}

void Editor::UnregisterSystem(std::string name, Inspectable* obj)
{
	auto& objs = Get().systemsObjs;
	const auto& it = objs.find(name);

	// Only removes if both 
	// - key is found 
	// - value is same as obj (to prevent accidentally removing other systems)
	if (it != objs.end() && it->second == obj)
		Get().systemsObjs.erase(name);
	else
		std::cout << "Failed to unregister system";
}

const Editor::EditorPrefs& Editor::GetEditorPrefs()
{
	return Get().editorPrefs;
}

void Editor::Update()
{
	Editor& instance = Get();
	if (AEInputCheckTriggered(AEVK_TAB))
		instance.showInspectors = !instance.showInspectors;

	if (AEInputCheckTriggered(AEVK_LBUTTON) && !ImGui::GetIO().WantCaptureMouse)
	{
		instance.focusedObject = nullptr;
		instance.showInspectors = false;

		AEVec2 mousePos;
		AEExtras::GetCursorWorldPosition(mousePos);
		for (Inspectable* obj : instance.gameObjs)
		{
			if (obj->CheckIfClicked(mousePos))
			{
				obj->isInspectorOpen = true;
				instance.focusedObject = obj;
				instance.showInspectors = true;
				break;
			}
		}
	}

	if (AEInputCheckTriggered(AEVK_LCTRL))
		instance.showColliders = !instance.showColliders;
}

void Editor::DrawInspectors()
{
	Editor& instance = Get();

	if (instance.showInspectors)
	{
		if (instance.showDemoWindow)
			ImGui::ShowDemoWindow(&instance.showDemoWindow);

		instance.DrawMenus();

		if (instance.focusedObject && instance.focusedObject->isInspectorOpen)
			instance.focusedObject->DrawInspector();

		for (auto& [name, obj] : instance.systemsObjs)
		{
			if (obj->isInspectorOpen)
				obj->DrawInspector();
		}
	}

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool Editor::GetShowColliders()
{
	return Get().showColliders;
}

Editor& Editor::Get()
{
	static Editor editor;
	return editor;
}

Editor::Editor() 
{
	LoadEditorPrefs();

	gsmSceneChangeEventId = EventSystem::Subscribe<SceneChangeEvent>([&](const SceneChangeEvent& ev) {
		OnSceneChange(ev);
	});
}

Editor::~Editor()
{
	SaveEditorPrefs();
	EventSystem::Unsubscribe<SceneChangeEvent>(gsmSceneChangeEventId);
}

void Editor::DrawMenus()
{
	Editor& instance = Get();
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::BeginMenu("Load Scene"))
			{
				for (int i = 0; i < SceneState::GS_SCENE_COUNT; i++)
				{
					SceneState state = static_cast<SceneState>(i);
					if (ImGui::MenuItem(GSM::GetStateName(state).c_str(), NULL))
					{
						GSM::ChangeScene(state);
						break;
					}
				}
				ImGui::EndMenu();
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Debug"))
		{
			ImGui::MenuItem("Show colliders", "Ctrl", &instance.showColliders);
			ImGui::MenuItem("Show Demo Window", NULL, &instance.showDemoWindow);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Systems"))
		{
			for (auto& [name, obj] : instance.systemsObjs)
				ImGui::MenuItem(name.c_str(), NULL, &obj->isInspectorOpen);

			ImGui::EndMenu();
		}

		float timeScale = Time::GetInstance().GetTimeScale(); // @todo - Replace this with time scale
		ImGui::SetNextItemWidth(200);
		if (ImGui::SliderFloat("Time Scale", &timeScale, 0, 2))
			Time::GetInstance().SetTimeScale(timeScale);


		ImGui::EndMainMenuBar();
	}
}

void Editor::LoadEditorPrefs()
{
	rapidjson::Document doc;
	bool success = FileHelper::TryReadJsonFile(editorPrefsPath, doc);
	if (!success)
		return;

	if (doc.HasMember("showColliders"))
		showColliders = doc["showColliders"].GetBool();

	if (doc.HasMember("lastOpenedScene"))
	{
		SceneState sceneState = static_cast<SceneState>(doc["lastOpenedScene"].GetInt());
		// Prevent loading QUIT or RESTART states. Something went wrong but not don't read for now
		if (sceneState >= 0)
			editorPrefs.lastOpenedScene = sceneState;
		else
			std::cout << "Failed to load scene editor prefs\n";
	}
}

void Editor::SaveEditorPrefs()
{
	rapidjson::Document doc;
	doc.SetObject();
	auto& allocator = doc.GetAllocator();
	doc.AddMember("lastOpenedScene", editorPrefs.lastOpenedScene, allocator);
	doc.AddMember("showColliders", showColliders, allocator);

	FileHelper::TryWriteJsonFile(editorPrefsPath, doc, true);
}

void Editor::OnSceneChange(const SceneChangeEvent& ev)
{
	editorPrefs.lastOpenedScene = ev.currentState;
	
	focusedObject = nullptr;

	SaveEditorPrefs();
}
