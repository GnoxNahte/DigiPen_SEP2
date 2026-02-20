#include "GSM.h"
#include "AEEngine.h"
#include "GameScene.h"
#include "MainMenuScene.h"
#include "../../Utils/QuickGraphics.h"
#include "../../../Saves/SaveSystem.h"
#include "../../../Saves/SaveData.h"
#include "../../Game/Timer.h"
#include "../../Game/Time.h"
#include "../../Game/UI.h"
#include "../../Editor/Editor.h"

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_win32.h>

#include "../../Utils/AEExtras.h" // temp

BaseScene* GSM::currentScene = nullptr;

// Set inital value. Not sure what to use, make it quit for now
SceneState GSM::previousState = GS_QUIT;
SceneState GSM::currentState = GS_QUIT;
SceneState GSM::nextState = GS_QUIT;

void GSM::Init(SceneState type)
{
	currentState = nextState = type;

	// === Init systems ===
	QuickGraphics::Init();
	SaveSystem::Init();
	Time::GetInstance();
	TimerSystem::GetInstance();
	UI::Init();
	// === Timer Testing ===
	//timerSystem.AddTimer("Test Timer 1", 3.0f);

	//// Test save
	//SaveData d;
	//d.levelId = 2;
	//d.hp = 88;
	//d.totalSeconds = 12.34f;
	//SaveSystem::Save(0, d);
}

void GSM::Update()
{
	// Similar to csd1130 GIT's game state manager
	while (currentState != GS_QUIT)
	{
		// If restart,
		if (currentState == GS_RESTART)
			currentState = nextState = previousState;
		// Else, load next state
		else
		{
			currentState = nextState;
			LoadState(currentState);
		}

		currentScene->Init();

		while (currentState == nextState)
		{
			// Informing the system about the loop's start
			AESysFrameStart();

			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			// Allows to dock window anywhere
			ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

			AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
			//AEGfxSetBackgroundColor(0.5f, 0.5f, 0.5f);

			AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);

			// Basic way to trigger exiting the application
			// when ESCAPE is hit or when the window is closed
			if (AEInputCheckTriggered(AEVK_ESCAPE) || 0 == AESysDoesWindowExist())
				nextState = GS_QUIT;

			currentScene->Update();
			Editor::Update();

			currentScene->Render();

			Time::GetInstance().Update();
			TimerSystem::GetInstance().Update();
			UI::GetDamageTextSpawner().Update();
			UI::Update();
			UI::Render();

			//// === For Damage Text Testing ===
			//if AEInputCheckCurr/Triggered
			if (AEInputCheckTriggered(AEVK_K))
			{
				AEVec2 pos{};
				pos.x = AEExtras::RandomRange({ 2.5f, 24.f });
				pos.y = AEExtras::RandomRange({ 2.5f, 10.f });
				DAMAGE_TYPE type = static_cast<DAMAGE_TYPE>(AEExtras::RandomRange({ 0,5 }));
				int dmg = static_cast<int>(AEExtras::RandomRange({ 1,1000 }));
				UI::GetDamageTextSpawner().SpawnDamageText(dmg, type, pos);
			}
			
			Editor::DrawInspectors();

			// Informing the system about the loop's end
			AESysFrameEnd();
		}

		currentScene->Exit();

		// Changing state
		if (nextState != GS_RESTART)
			delete currentScene;

		previousState = currentState;
		currentState = nextState;
	}
}

void GSM::Exit()
{
	QuickGraphics::Free();
	UI::Exit();
}

void GSM::ChangeScene(SceneState state)
{
	if (currentState == state)
		nextState = GS_RESTART;
	else
		nextState = state;
}

SceneState GSM::GetState()
{
	return currentState;
}

std::string GSM::GetStateName(SceneState state)
{
	switch (state)
	{
		case GS_QUIT: return "QUIT";
		case GS_RESTART: return "RESTART";
		case GS_SPLASH_SCREEN: return "Splash Screen";
		case GS_MAIN_MENU: return "Main Menu";
		case GS_GAME: return "Game";

		default: return "Unknown";
	}
}

void GSM::LoadState(SceneState state)
{
	// @todo add other states.
	switch (state)
	{
		//case GS_SPLASH_SCREEN: currentScene = new SplashScreenScene(); break;
		case GS_MAIN_MENU:	currentScene = new MainMenuScene();	break;
		case GS_GAME:		currentScene = new GameScene();		break;
		default:			std::cout << "Loading to unknown scene.\n"; break;
	}
}
