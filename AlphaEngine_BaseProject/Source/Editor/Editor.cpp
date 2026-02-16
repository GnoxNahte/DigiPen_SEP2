#include "Editor.h"
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <AEEngine.h>

#include "../Game/Scene/GSM.h"
#include "../Utils/AEExtras.h"

void Editor::Register(Inspectable& obj)
{
	Get().menuObjs.emplace_back(std::ref(obj));
}

void Editor::Unregister(Inspectable& obj)
{
	auto& menu = Get().menuObjs;
	std::erase_if(menu, [&obj](const auto& ref) {
		return &ref.get() == &obj;
	});
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
		for (Inspectable& obj : instance.menuObjs)
		{
			if (obj.CheckIfClicked(mousePos))
			{
				instance.focusedObject = &obj;
				instance.showInspectors = true;
				break;
			}
		}
	}

	instance.showColliders = AEInputCheckCurr(AEVK_LCTRL);
}

void Editor::DrawInspectors()
{
	Editor& instance = Get();

	if (instance.showInspectors)
	{
		if (instance.showDemoWindow)
			ImGui::ShowDemoWindow(&instance.showDemoWindow);

		instance.DrawMenus();

		if (instance.focusedObject)
			instance.focusedObject->DrawInspector();
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

			if (ImGui::MenuItem("Show Demo Window", NULL, &instance.showDemoWindow))
				ImGui::ShowDemoWindow();
			
			ImGui::EndMenu();
		}

		static float timeScale = 0; // @todo - Replace this with time scale
		ImGui::SetNextItemWidth(200);
		ImGui::SliderFloat("Time Scale", &timeScale, 0, 2);


		ImGui::EndMainMenuBar();
	}

}
