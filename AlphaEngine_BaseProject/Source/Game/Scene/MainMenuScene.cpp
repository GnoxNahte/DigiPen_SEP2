#include "MainMenuScene.h"
#include "AEEngine.h"
#include "../../MainMenu.h"

#include <string>

// shared selected level path (game scene should read this when entering GS_GAME)
std::string gPendingLevelPath;

MainMenuScene::MainMenuScene() {}
MainMenuScene::~MainMenuScene() {}

void MainMenuScene::Init()
{
    MainMenu::Init();
}

void MainMenuScene::Update()
{
    const int w = (int)AEGfxGetWindowWidth();
    const int h = (int)AEGfxGetWindowHeight();

    s32 mx, my;
    AEInputGetCursorPosition(&mx, &my);

    MainMenu::Update(w, h, mx, my, AEInputCheckTriggered(AEVK_LBUTTON));

    std::string path;
    if (MainMenu::ConsumeStartRequest(path))
    {
        gPendingLevelPath = path;
        GSM::ChangeScene(SceneState::GS_GAME);
        return;
    }

    if (MainMenu::ConsumeQuitRequest())
    {
        AEGfxExit();
        return;
    }
}

void MainMenuScene::Render()
{
    MainMenu::Render((int)AEGfxGetWindowWidth(), (int)AEGfxGetWindowHeight());
}

void MainMenuScene::Exit()
{
    MainMenu::Shutdown();
}