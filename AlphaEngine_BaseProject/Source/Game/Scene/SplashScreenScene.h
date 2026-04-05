/*!
@file	SplashScreenScene.h
@author	Ethan Ong
@brief	Declares the SplashScreenScene class 
		for drawing the DigiPen Logo and game name

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/

#pragma once
#include "GSM.h"
#include <AEEngine.h>

class SplashScreenScene : public BaseScene
{
public:
	SplashScreenScene();
	~SplashScreenScene();

	void Init() override;
	void Update() override;
	void Render() override;
	void Exit() override;

private:
	enum State
	{
		FADE_IN,
		STAY,
		FADE_OUT,

		NEXT_SCENE,
	};

	const float stayDuration = 1.5f;
	const float fadeDuration = 1.5f;

	bool isShowingLogo = true; // Showing DigiPen logo
	State currState = FADE_IN;
	f64 timeLeft = -1.f; // time left in current state

	float transparency = 1.f;
	
	AEGfxVertexList* mesh;
	AEGfxTexture* texture;
	
	s8 fontId;
};

