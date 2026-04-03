#include "SplashScreenScene.h"
#include "../../Utils/MeshGenerator.h"
#include "../../Utils/AEExtras.h"

SplashScreenScene::SplashScreenScene()
{
	texture = AEGfxTextureLoad("Assets/Art/DigiPen_Singapore_Logo_WithCopyright.png");
	mesh = MeshGenerator::GetSquareMesh(1.f);

    fontId = AEGfxCreateFont("Assets/m04.ttf", 36);
}

SplashScreenScene::~SplashScreenScene()
{
	AEGfxMeshFree(mesh);
	AEGfxTextureUnload(texture);

    AEGfxDestroyFont(fontId);
}

void SplashScreenScene::Init()
{
    isShowingLogo = true;
    currState = FADE_IN;
    timeLeft = fadeDuration;
}

void SplashScreenScene::Update()
{
    bool ifSwitchNextScene = false;

    timeLeft -= AEFrameRateControllerGetFrameTime();
    if (timeLeft < 0)
    {
        if (currState == State::FADE_OUT)
        {
            if (isShowingLogo)
            {
                isShowingLogo = false;
                currState = State::FADE_IN;
                timeLeft = fadeDuration;
            }
            else
            {
                ifSwitchNextScene = true;
            }
        }
        else
        {
            currState = static_cast<State>(currState + 1);
            timeLeft = currState == State::STAY ? stayDuration : fadeDuration;
        }
    }

    ifSwitchNextScene |= AEInputCheckCurr(AEVK_ESCAPE) || AEInputCheckCurr(AEVK_SPACE) || AEInputCheckCurr(AEVK_RETURN);
    if (ifSwitchNextScene)
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

    if (isShowingLogo)
    {
        AEGfxTextureSet(texture, 1.f, 1.f);
        AEGfxSetTransparency(transparency);

        float width = static_cast<float>(AEGfxGetWindowWidth());
        float height = width / 1280.f * 720.f; // Multiply by aspect ratio of texture
        AEMtx33 transform;
        AEMtx33Scale(&transform, width, height); 
        AEGfxSetTransform(transform.m);

        AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
    }
    else
    {
        f32 width, height;
        const char* str = "AETHERFALL";
        AEGfxGetPrintSize(fontId, str, 1.f, &width, &height);
        AEGfxPrint(fontId, str, -width * 0.5f, -height * 0.5f, 1, 1, 1, 1, transparency);
    }

    // Reset
    AEGfxSetTransparency(1.f);
}

void SplashScreenScene::Exit()
{
}
