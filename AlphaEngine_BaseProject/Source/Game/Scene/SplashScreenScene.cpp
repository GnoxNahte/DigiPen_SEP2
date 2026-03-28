#include "SplashScreenScene.h"
#include "../../Utils/MeshGenerator.h"
#include "../../Utils/AEExtras.h"

SplashScreenScene::SplashScreenScene()
{
	texture = AEGfxTextureLoad("Assets/DigiPen_Singapore_WEB_RED.png");
	mesh = MeshGenerator::GetSquareMesh(1.f);
}

SplashScreenScene::~SplashScreenScene()
{
	AEGfxMeshFree(mesh);
	AEGfxTextureUnload(texture);
}

void SplashScreenScene::Init()
{
    timeLeft = fadeDuration;
}

void SplashScreenScene::Update()
{
    timeLeft -= AEFrameRateControllerGetFrameTime();
    if (timeLeft < 0)
    {
        currState = static_cast<State>(currState + 1);
        timeLeft = currState == State::STAY ? stayDuration : fadeDuration;
    }

    if (currState == NEXT_SCENE || AEInputCheckCurr(AEVK_ESCAPE) || AEInputCheckCurr(AEVK_SPACE) || AEInputCheckCurr(AEVK_RETURN))
        GSM::ChangeScene(SceneState::GS_MAIN_MENU);

    switch (currState)
    {
    case SplashScreenScene::FADE_IN:    transparency = 1.f - static_cast<float>(timeLeft) / fadeDuration; break;
    case SplashScreenScene::STAY:       transparency = 1.f; break;
    case SplashScreenScene::FADE_OUT:   transparency = static_cast<float>(timeLeft) / fadeDuration; break;
    }
}

void SplashScreenScene::Render()
{
    AEGfxSetCamPosition(0.f, 0.f);
    AEGfxSetBackgroundColor(0.f, 0.f, 0.f);

    AEGfxTextureSet(texture, 1.f, 1.f);
    AEGfxSetTransparency(transparency);

    float width = static_cast<float>(AEGfxGetWindowWidth());
    float height = width / 1525.f * 445.f;
    AEMtx33 transform;
    AEMtx33Scale(&transform, width, height); 
    AEGfxSetTransform(transform.m);

    AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);

    // Reset
    AEGfxSetTransparency(1.f);
}

void SplashScreenScene::Exit()
{
}
