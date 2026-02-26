#include "GSM.h"
#include "AEEngine.h"
#include "GameScene.h"
#include "MainMenuScene.h"
#include "../../Utils/QuickGraphics.h" 
#include "../../../Saves/SaveSystem.h"
#include "../../../Saves/SaveData.h"
#include "../../Game/Timer.h"
#include "../../Game/Time.h"
#include "LevelEditorScene.h"
#include "../../Editor/Editor.h"

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_win32.h>
#include "../../Game/BuffCards.h"

#include "../../Utils/AEExtras.h" // temp

BaseScene* GSM::currentScene = nullptr;

// Set inital value. Not sure what to use, make it quit for now
SceneState GSM::previousState = GS_QUIT;
SceneState GSM::currentState = GS_QUIT;
SceneState GSM::nextState = GS_QUIT;
static bool gPauseMenuOpen = false;
static bool gPauseShowSettings = false;


void GSM::Init(SceneState type)
{
	currentState = nextState = type;

	// === Init systems ===
	QuickGraphics::Init();
	SaveSystem::Init();
	Time::GetInstance();
	TimerSystem::GetInstance();
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

		gPauseMenuOpen = false;
		gPauseShowSettings = false;
		Time::GetInstance().SetPaused(false);

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

			AEGfxSetBackgroundColor(0.129f, 0.114f, 0.18f);
			//AEGfxSetBackgroundColor(0.5f, 0.5f, 0.5f);

			AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);

			// Window closed => quit no matter what
			if (0 == AESysDoesWindowExist())
				nextState = GS_QUIT;

			// ESC => open/close pause menu ONLY in game
			if (currentState == GS_GAME && AEInputCheckTriggered(AEVK_ESCAPE))
			{
				gPauseMenuOpen = !gPauseMenuOpen;
				gPauseShowSettings = false; // reset to main pause page
				Time::GetInstance().SetPaused(gPauseMenuOpen);
			}
			else if (currentState != GS_GAME && AEInputCheckTriggered(AEVK_ESCAPE))
			{
				// optional: outside gameplay, ESC quits
				nextState = GS_QUIT;
			}

			// Freeze gameplay updates when pause menu is open
			if (!gPauseMenuOpen)
			{
				currentScene->Update();
			}

			Editor::Update();
			currentScene->Render();

			// Draw pause menu overlay (ImGui)
			if (gPauseMenuOpen && currentState == GS_GAME)
			{
				ImGuiViewport* mainVp = ImGui::GetMainViewport();

				// ===== Full-screen dim overlay =====
				ImGui::SetNextWindowPos(mainVp->Pos, ImGuiCond_Always);
				ImGui::SetNextWindowSize(mainVp->Size, ImGuiCond_Always);
				ImGui::SetNextWindowBgAlpha(0.85f);
				ImGui::Begin("##PauseDimOverlay", nullptr,
					ImGuiWindowFlags_NoDecoration |
					ImGuiWindowFlags_NoMove |
					ImGuiWindowFlags_NoSavedSettings |
					ImGuiWindowFlags_NoBringToFrontOnFocus);
				ImGui::End();

				// ===== Buff bar (bottom-right) =====
				{
					const auto& buffs = BuffCardManager::GetCurrentBuffs();
					if (!buffs.empty())
					{
						const float CARD_W = 78.0f;
						const float CARD_H = 110.0f;
						const float PAD = 16.0f;
						const float MARGIN = 70.0f;

						const int count = (int)buffs.size();

						const float winWf = count * CARD_W + (count - 1) * PAD;
						const float winHf = CARD_H;

						const float posXf = mainVp->Pos.x + mainVp->Size.x - winWf - MARGIN;
						const float posYf = mainVp->Pos.y + mainVp->Size.y - winHf - MARGIN;

						const float winW = (float)(int)(winWf + 0.5f);
						const float winH = (float)(int)(winHf + 0.5f);
						const float posX = (float)(int)(posXf + 0.5f);
						const float posY = (float)(int)(posYf + 0.5f);

						ImGui::SetNextWindowPos(ImVec2(posX, posY), ImGuiCond_Always);
						ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_Always);
						ImGui::SetNextWindowBgAlpha(0.25f);

						ImGui::Begin("##PauseBuffBar", nullptr,
							ImGuiWindowFlags_NoDecoration |
							ImGuiWindowFlags_NoMove |
							ImGuiWindowFlags_NoSavedSettings |
							ImGuiWindowFlags_NoFocusOnAppearing |
							ImGuiWindowFlags_NoNav);

						// 关键：Push/Pop 必须成对
						ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
						ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(PAD, 0.0f));

						for (int i = 0; i < count; ++i)
						{
							if (i > 0) ImGui::SameLine();

							const BuffCard& b = buffs[i];
							std::string id = "##buff_" + std::to_string(i);

							ImGui::Button(id.c_str(), ImVec2(CARD_W, CARD_H));

							if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
							{
								ImGui::BeginTooltip();
								ImGui::Text("%s", b.cardName.c_str());
								ImGui::Separator();
								ImGui::Text("Type: %s", BuffCardManager::CardTypeToString(b.type).c_str());
								ImGui::Text("Rarity: %s", BuffCardManager::CardRarityToString(b.rarity).c_str());
								ImGui::Separator();
								ImGui::TextWrapped("%s", b.cardDesc.c_str());
								ImGui::TextWrapped("Effect: %s", b.cardEffect.c_str());
								ImGui::Text("Value1: %d", b.effectValue1);
								ImGui::Text("Value2: %d", b.effectValue2);
								ImGui::EndTooltip();
							}
						}

						ImGui::PopStyleVar(2);
						ImGui::End();
					}
				}

				// ===== Pause menu window (center) =====
				ImGui::SetNextWindowBgAlpha(0.70f);
				ImVec2 center(mainVp->Pos.x + mainVp->Size.x * 0.5f, mainVp->Pos.y + mainVp->Size.y * 0.5f);
				ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));

				ImGui::Begin("Pause Menu", nullptr,
					ImGuiWindowFlags_NoDecoration |
					ImGuiWindowFlags_AlwaysAutoResize |
					ImGuiWindowFlags_NoMove);

				if (!gPauseShowSettings)
				{
					ImGui::Text("PAUSED");
					ImGui::Separator();

					if (ImGui::Button("Resume", ImVec2(220, 0)))
					{
						gPauseMenuOpen = false;
						Time::GetInstance().SetPaused(false);
					}
					if (ImGui::Button("Settings", ImVec2(220, 0)))
					{
						gPauseShowSettings = true;
					}
					if (ImGui::Button("Exit", ImVec2(220, 0)))
					{
						nextState = GS_QUIT;
					}
				}
				else
				{
					ImGui::Text("SETTINGS");
					ImGui::Separator();

					static float masterVolume = 1.0f;
					ImGui::SliderFloat("Master Volume", &masterVolume, 0.0f, 1.0f);

					if (ImGui::Button("Back", ImVec2(220, 0)))
					{
						gPauseShowSettings = false;
					}
				}

				ImGui::End();
			}

			Time::GetInstance().Update();
			TimerSystem::GetInstance().Update();

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
		case GS_MAIN_MENU: currentScene = new MainMenuScene(); break;
		case GS_GAME:	currentScene = new GameScene();		break;
		case GS_LEVEL_EDITOR:   currentScene = new LevelEditorScene(); break;
			
		default:		std::cout << "Loading to unknown scene.\n"; break;
	}
}
