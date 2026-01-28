#include <iostream>
#include "MainMenuScene.h"
#include "AEEngine.h"

MainMenuScene::MainMenuScene()
{
	std::cout << "Main menu load\n";
}

MainMenuScene::~MainMenuScene()
{
	std::cout << "Main menu unload\n";
}

void MainMenuScene::Init()
{
	std::cout << "Main menu Init\n";
}

void MainMenuScene::Update()
{
	std::cout << "Main menu update\n";
	if (AEInputCheckTriggered(AEVK_R))
		GSM::ChangeScene(SceneState::GS_GAME);
}

void MainMenuScene::Render()
{
	std::cout << "Main menu render\n";
}

void MainMenuScene::Exit()
{
	std::cout << "Main menu Exit\n";
}
