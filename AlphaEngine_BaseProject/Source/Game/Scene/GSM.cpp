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

BaseScene* GSM::currentScene = nullptr;

// Set inital value. Not sure what to use, make it quit for now
SceneState GSM::previousState = GS_QUIT;
SceneState GSM::currentState = GS_QUIT;
SceneState GSM::nextState = GS_QUIT;

//f32 alpha = 1.f;
//f32 scale = 1.f;
//int damageType = 0; // Testing of cycling through enum types.

void GSM::Init(SceneState type)
{
	currentState = nextState = type;

	// === Init systems ===
	QuickGraphics::Init();
	SaveSystem::Init();
	Time::GetInstance();
	TimerSystem::GetInstance();
	/*UI::InitDamageFont("Assets/Bemock.ttf", 48, 52);*/
	UI::Init();

	// === Damage text testing variables ===
	//f32 alpha = 1.f;
	//f32 scale = 1.f;
	//int damageType = 0; // Testing of cycling through enum types.

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
			UI::Render();

			//// === For Damage Text Testing ===
			//if (AEInputCheckTriggered(AEVK_K)) {
			//	TimerSystem::GetInstance().AddTimer("Damage Scale Timer", 0.28f);
			//	TimerSystem::GetInstance().AddTimer("Damage Fade Timer", 1.5f);
			//	damageType = (damageType + 1) % 6; // enum testing
			//	alpha = 1.0f;

			//}
			//if (TimerSystem::GetInstance().GetTimerByName("Damage Scale Timer") != nullptr && TimerSystem::GetInstance().GetTimerByName("Damage Scale Timer")->percentage < 1.f)
			//{
			//	scale = static_cast<f32>(1 - TimerSystem::GetInstance().GetTimerByName("Damage Scale Timer")->percentage + 0.95f);
			//}
			//if (TimerSystem::GetInstance().GetTimerByName("Damage Fade Timer") != nullptr) {
			//	alpha = static_cast<f32>(1 - 1.0f * TimerSystem::GetInstance().GetTimerByName("Damage Fade Timer")->percentage);
			//	if (TimerSystem::GetInstance().GetTimerByName("Damage Fade Timer")->percentage < 1.0f) {
			//		UI::PrintDamageText(178, { -0.5f, 0.5f }, scale, alpha, damageType);
			//	}
			//	else {
			//		alpha = 1.f;
			//	}
			//}
			// === For Timers Testing ===
			// Can use this as a sample test case if you want to use timers.
			//if (AEInputCheckTriggered(AEVK_K)) {
			//	TimerSystem::GetInstance().AddAnonymousTimer(2.0f);
			//	TimerSystem::GetInstance().AddAnonymousTimer(5.5f, true, false, false); // Exposing all default params
			//	Time::GetInstance().TogglePause(); // Set it to paused state to test timers that are not ignoring pause state
			//	TimerSystem::GetInstance().AddTimer("Test timer", 5.0f, false, true, true, true, 2); // Add a timer that ignores pause to unpause after 2 iterations.
			//}

			//Timer const* timer = TimerSystem::GetInstance().GetTimerByName("Test timer");
			//if (timer) {
			//	// After it loops loopCount times it will not reset its completion, unpausing the state and allowing other timers to countdown.
			//	if (timer->completed) {
			//		Time::GetInstance().TogglePause(); // Unpause
			//		TimerSystem::GetInstance().RemoveTimer("Test timer"); // Delete the timer after use
			//		std::cout << "Unpaused!" << '\n';
			//	}
			//}
			
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
