#pragma once
#include "GSM.h"

// Main menu scene just for testing
class MainMenuScene : public BaseScene
{
public:
	MainMenuScene();
	~MainMenuScene();
	void Init() override;
	void Update() override;
	void Render() override;
	void Exit() override;
};
