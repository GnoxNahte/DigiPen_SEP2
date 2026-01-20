// ---------------------------------------------------------------------------
// includes
#pragma once
#pragma message("Including SaveSystem.h from: " __FILE__)
#include <crtdbg.h> // To check for memory leaks
#include "AEEngine.h"

#include "Game/GameScene.h"
#include "Utils/QuickGraphics.h"
#include "../Saves/SaveSystem.h"
#include "../Saves/SaveData.h"
#include "Game/Timer.h"
#include "Game/UI.h"




// ---------------------------------------------------------------------------
// main

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// ===== Setup window and AlphaEngine =====
	int gGameRunning = 1;

	// Using custom window procedure
	AESysInit(hInstance, nCmdShow, 1600, 900, 1, 120, false, NULL);
	
	// Changing the window title
	AESysSetWindowTitle("GAM 150"); // @todo: change name

	// reset the system modules
	AESysReset();

	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
	AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
	AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetTransparency(1.0f);

	// === Init systems ===
	QuickGraphics::Init();
	SaveSystem::Init();
	TimerSystem timerSystem;
	UI::InitDamageFont("Assets/Bemock.ttf", 48, 52);

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


	// === Game loop ===
	GameScene scene;
	
	// Game Loop
	while (gGameRunning)
	{
		// Informing the system about the loop's start
		AESysFrameStart();

		AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
		//AEGfxSetBackgroundColor(0.5f, 0.5f, 0.5f);

		AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);

		// Basic way to trigger exiting the application
		// when ESCAPE is hit or when the window is closed
		if (AEInputCheckTriggered(AEVK_ESCAPE) || 0 == AESysDoesWindowExist())
			gGameRunning = 0;

		scene.Update();
		scene.Render();

		timerSystem.Update();

		// === For Damage Text Testing ===
		//if (AEInputCheckTriggered(AEVK_K)) {
		//	timerSystem.AddTimer("Damage Scale Timer", 0.28f);
		//	timerSystem.AddTimer("Damage Fade Timer", 1.5f);
		//	damageType = (damageType + 1) % 6; // enum testing
		//	alpha = 1.0f;
		//	
		//}
		//if (timerSystem.GetTimerByName("Damage Scale Timer") != nullptr && timerSystem.GetTimerByName("Damage Scale Timer")->percentage < 1.f)
		//{
		//	scale = static_cast<f32>(1 - timerSystem.GetTimerByName("Damage Scale Timer")->percentage + 0.95f);
		//}
		//if (timerSystem.GetTimerByName("Damage Fade Timer") != nullptr) {
		//	alpha = static_cast<f32>(1 - 1.0f * timerSystem.GetTimerByName("Damage Fade Timer")->percentage);
		//	if (timerSystem.GetTimerByName("Damage Fade Timer")->percentage < 1.0f) {
		//		UI::PrintDamageText(178, { -0.5f, 0.5f }, scale, alpha, damageType);
		//	}
		//	else {
		//		alpha = 1.f;
		//	}
		//}

		// Informing the system about the loop's end
		AESysFrameEnd();

	}
	timerSystem.Clear();
	// free the system
	AESysExit();
}