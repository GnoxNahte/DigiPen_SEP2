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

	float stayDuration = 1.f;
	float fadeDuration = 1.5f;

	State currState = FADE_IN;
	f64 timeLeft = -1.f; // time left in current state

	float transparency = 1.f;

	AEGfxVertexList* mesh;
	AEGfxTexture* texture;
};

