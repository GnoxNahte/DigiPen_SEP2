#include "Editor.h"
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <AEEngine.h>

Inspectable::Inspectable()
{
	Editor::Register(*this);
}

Inspectable::~Inspectable()
{
	Editor::Unregister(*this);
}

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
	if (Editor& instance = Get(); AEInputCheckTriggered(AEVK_TAB))
	{
		if (AEInputCheckCurr(AEVK_LSHIFT))
			instance.showDemoWindow = !instance.showDemoWindow;
		else
			instance.showInspectors = !instance.showInspectors;
	}
}

void Editor::DrawInspectors()
{
	Editor& instance = Get();

	if (instance.showInspectors)
	{
		if (instance.showDemoWindow)
			ImGui::ShowDemoWindow(&instance.showDemoWindow);

		//ImGui::BeginMainMenuBar();
		//
		//ImGui::EndMainMenuBar();
		
		for (Inspectable& obj : instance.menuObjs)
			obj.DrawInspector();
	}
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

Editor& Editor::Get()
{
	static Editor editor;
	return editor;
}

Editor::Editor() 
{
}
