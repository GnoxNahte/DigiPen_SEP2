// ---------------------------------------------------------------------------
// includes
#pragma once
#pragma message("Including SaveSystem.h from: " __FILE__)
#include <crtdbg.h> // To check for memory leaks
#include "AEEngine.h"
#include "Game/Scene/GSM.h"




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

	// Using custom window procedure
	AESysInit(hInstance, nCmdShow, 1600, 900, 1, 240, false, NULL);

	// Changing the window title
	AESysSetWindowTitle("GAM 150"); // @todo: change name

	// reset the system modules
	AESysReset();

	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
	AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
	AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetTransparency(1.0f);

	//GSM::Init(SceneState::GS_SPLASH_SCREEN);
	GSM::Init(SceneState::GS_GAME);
	GSM::Update();
	GSM::Exit();

	// free the system
	AESysExit();
}