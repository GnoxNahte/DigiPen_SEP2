// ---------------------------------------------------------------------------
// includes
#pragma once
#pragma message("Including SaveSystem.h from: " __FILE__)
#include <crtdbg.h> // To check for memory leaks
#include "AEEngine.h"
#include "Game/Scene/GSM.h"

#include <iostream>

#include <windows.h>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_opengl3.h>

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

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
	AESysInit(hInstance, nCmdShow, 1600, 900, 1, 240, false, WndProc);
	
	// Changing the window title
	AESysSetWindowTitle("GAM 150"); // @todo: change name

	// reset the system modules
	AESysReset();

	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
	AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
	AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetTransparency(1.0f);

	// === ImGui setup ===
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;    // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;       // Enable Docking
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;     // Enable Multi-Viewport / Platform Windows

	//ImGui::StyleColorsDark();

	// Setup scaling
	float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));
	ImGuiStyle& style = ImGui::GetStyle();
	style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
	style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)
	io.ConfigDpiScaleFonts = true;          // [Experimental] Automatically overwrite style.FontScaleDpi in Begin() when Monitor DPI changes. This will scale fonts but _NOT_ scale sizes/padding for now.
	io.ConfigDpiScaleViewports = true;      // [Experimental] Scale Dear ImGui and Platform Windows when Monitor DPI changes.

	//// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	//if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	//{
	//	style.WindowRounding = 0.0f;
	//	style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	//}

	HWND hwnd = AESysGetWindowHandle();
	ImGui_ImplWin32_InitForOpenGL(hwnd);
	ImGui_ImplOpenGL3_Init();

	//// Win32+GL needs specific hooks for viewport, as there are specific things needed to tie Win32 and GL api.
	//if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	//{
	//	ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
	//	IM_ASSERT(platform_io.Renderer_CreateWindow == NULL);
	//	IM_ASSERT(platform_io.Renderer_DestroyWindow == NULL);
	//	IM_ASSERT(platform_io.Renderer_SwapBuffers == NULL);
	//	IM_ASSERT(platform_io.Platform_RenderWindow == NULL);
	//	platform_io.Renderer_CreateWindow = Hook_Renderer_CreateWindow;
	//	platform_io.Renderer_DestroyWindow = Hook_Renderer_DestroyWindow;
	//	platform_io.Renderer_SwapBuffers = Hook_Renderer_SwapBuffers;
	//	platform_io.Platform_RenderWindow = Hook_Platform_RenderWindow;
	//}

	//GSM::Init(SceneState::GS_SPLASH_SCREEN);
	GSM::Init(SceneState::GS_GAME);
	GSM::Update();
	GSM::Exit();

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	//CleanupDeviceWGL(hwnd, &g_MainWindow);
	//wglDeleteContext(g_hRC);
	//::DestroyWindow(hwnd);
	//::UnregisterClassW(wc.lpszClassName, wc.hInstance);

	// free the system
	AESysExit();
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	//std::cout << std::hex << "MSG: " << msg << "\n";
	switch (msg)
	{
	case WM_SIZE:
		/*if (wParam != SIZE_MINIMIZED)
		{
			g_Width = LOWORD(lParam);
			g_Height = HIWORD(lParam);
		}*/

		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		GSM::ChangeScene(SceneState::GS_QUIT);
		::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}