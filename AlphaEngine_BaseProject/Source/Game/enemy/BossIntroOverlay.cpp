#include "BossIntroOverlay.h"

#include "../GameOver.h"
#include "../Camera.h"
#include "../../Utils/MeshGenerator.h"
#include "../../Utils/ParticleSystem.h"

namespace
{
    enum class BossIntroPhase
    {
        None,
        Black,
        TextFadeIn,
        TextHold,
        EyelidOpen
    };

    // resources owned by this overlay
    s8 gFont = 0;
    AEGfxVertexList* gRectMesh = nullptr;
    ParticleSystem* gPurpleParticles = nullptr;

    // state
    bool gActive = false;
    BossIntroPhase gPhase = BossIntroPhase::None;

    float gTimer = 0.0f;
    float gTextAlpha = 0.0f;
    float gEyeAlpha = 0.0f;

    // tweakables
    const int   BOSS_INTRO_FONT_SIZE = 48;
    const char* INTRO_TEXT_LEFT = "What an ";
    const char* INTRO_TEXT_RED = "ominous";
    const char* INTRO_TEXT_RIGHT = " feeling....";

    // text placement in NDC
    const float TEXT_X = -0.62f;
    const float TEXT_Y = 0.05f;
    const float TEXT_SCALE = 0.9f;
    const float TEXT_RED_OFFSET_X = 0.23f;
    const float TEXT_RIGHT_OFFSET_X = 0.43f;

    // red eye position in screen-space pixels from center
    const float EYE_OFFSET_X = 36.0f;
    const float EYE_OFFSET_Y = 14.0f;

    // phase timings
    const float BLACK_DURATION = 0.8f;
    const float TEXT_FADE_DURATION = 1.8f;
    const float TEXT_HOLD_DURATION = 0.9f;
    const float EYELID_OPEN_DURATION_HINT = 1.f;

    ParticleSystem::EmitterSettings MakePurpleEmitter()
    {
        ParticleSystem::EmitterSettings e{};

        // overwritten every frame to follow the camera center
        e.spawnPosRangeX = { -1.0f, 1.0f };
        e.spawnPosRangeY = { -1.0f, 1.0f };

        // faint slow upward drift
        e.angleRange = { AEDegToRad(75.0f), AEDegToRad(105.0f) };
        e.speedRange = { 0.15f, 0.40f };
        e.lifetimeRange = { 0.90f, 1.60f };

        e.tint = { 0.50f, 0.15f, 0.80f, 0.20f };
        e.behavior = ParticleBehavior::normal;
        //choose (2,10), (2,3), (2,5), im not sure which looks the best?
        e.sizeRange = {2.f, 5.f };

        return e;
    }

    void SyncPurpleEmitterToCamera()
    {
        if (!gPurpleParticles)
            return;

        const float safeScale = (Camera::scale > 0.001f) ? Camera::scale : 0.001f;

   
        // x: about +/-140 px, y: about +/-85 px
        const float halfW = 140.0f / safeScale;
        const float halfH = 85.0f / safeScale;

        const float cx = Camera::position.x;
        const float cy = Camera::position.y;

        auto& e = gPurpleParticles->emitter;
        e.spawnPosRangeX = { cx - halfW, cx + halfW };
        e.spawnPosRangeY = { cy - halfH, cy + halfH };
    }

    void UpdatePurpleParticles()
    {
        if (!gPurpleParticles)
            return;

        SyncPurpleEmitterToCamera();

        if (gPhase == BossIntroPhase::Black ||
            gPhase == BossIntroPhase::TextFadeIn ||
            gPhase == BossIntroPhase::TextHold)
        {
            gPurpleParticles->SetSpawnRate(26.0f);
        }
        else
        {
            // stop spawning during the eyelid-open phase,
            // but let existing particles finish naturally
            gPurpleParticles->SetSpawnRate(0.0f);
        }

        gPurpleParticles->Update();
    }

    void RenderPurpleParticles()
    {
        if (!gPurpleParticles)
            return;

        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        gPurpleParticles->Render();
    }

    void DrawScreenRectPx(float centerX, float centerY, float width, float height,
        float r, float g, float b, float a)
    {
        if (!gRectMesh || a <= 0.0f)
            return;

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(a);
        AEGfxSetColorToMultiply(r, g, b, 1.0f);
        AEGfxSetColorToAdd(0, 0, 0, 0);

        AEMtx33 scale, trans, transform;
        AEMtx33Scale(&scale, width, height);
        AEMtx33Trans(&trans,
            Camera::position.x * Camera::scale + centerX,
            Camera::position.y * Camera::scale + centerY);
        AEMtx33Concat(&transform, &trans, &scale);

        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(gRectMesh, AE_GFX_MDM_TRIANGLES);
    }

    void RenderEyeFlash()
    {
        if (gEyeAlpha <= 0.0f)
            return;

        // outer glow
        DrawScreenRectPx(
            EYE_OFFSET_X, EYE_OFFSET_Y,
            54.0f, 10.0f,
            0.75f, 0.00f, 0.00f,
            gEyeAlpha * 0.55f
        );

        // inner bright slit
        DrawScreenRectPx(
            EYE_OFFSET_X, EYE_OFFSET_Y,
            28.0f, 4.0f,
            1.00f, 0.08f, 0.08f,
            gEyeAlpha
        );
    }

    void RenderIntroText()
    {
        if (gTextAlpha <= 0.0f)
            return;

        AEGfxPrint(gFont, INTRO_TEXT_LEFT,
            TEXT_X, TEXT_Y, TEXT_SCALE,
            1.0f, 1.0f, 1.0f, gTextAlpha);

        AEGfxPrint(gFont, INTRO_TEXT_RED,
            TEXT_X + TEXT_RED_OFFSET_X, TEXT_Y, TEXT_SCALE,
            0.75f, 0.08f, 0.08f, gTextAlpha);

        AEGfxPrint(gFont, INTRO_TEXT_RIGHT,
            TEXT_X + TEXT_RIGHT_OFFSET_X, TEXT_Y, TEXT_SCALE,
            1.0f, 1.0f, 1.0f, gTextAlpha);
    }
}

namespace BossIntroOverlay
{
    void Init()
    {
        if (!gFont)
            gFont = AEGfxCreateFont("Assets/Pixellari.ttf", BOSS_INTRO_FONT_SIZE);

        if (!gRectMesh)
            gRectMesh = MeshGenerator::GetRectMesh(1.0f, 1.0f);

        if (!gPurpleParticles)
        {
            gPurpleParticles = new ParticleSystem(96, MakePurpleEmitter());
            gPurpleParticles->Init();
            gPurpleParticles->SetSpawnRate(0.0f);
        }

        Reset();
    }

    void Exit()
    {
        if (gPurpleParticles)
        {
            delete gPurpleParticles;
            gPurpleParticles = nullptr;
        }

        if (gRectMesh)
        {
            AEGfxMeshFree(gRectMesh);
            gRectMesh = nullptr;
        }

        if (gFont)
        {
            AEGfxDestroyFont(gFont);
            gFont = 0;
        }

        gActive = false;
        gPhase = BossIntroPhase::None;
        gTimer = 0.0f;
        gTextAlpha = 0.0f;
        gEyeAlpha = 0.0f;
    }

    void Start()
    {
        gActive = true;
        gPhase = BossIntroPhase::Black;

        gTimer = 0.0f;
        gTextAlpha = 0.0f;
        gEyeAlpha = 0.0f;

        if (gPurpleParticles)
        {
            gPurpleParticles->ReleaseAll();
            gPurpleParticles->Init();
            SyncPurpleEmitterToCamera();

            // immediate presence at frame 1 instead of waiting for spawn rate
            gPurpleParticles->SpawnParticleBurst(14);
            gPurpleParticles->SetSpawnRate(26.0f);
        }

        // fully black at the start
        SetEyelidProgress(GetEyelidMaxProgress());
    }

    void Reset()
    {
        gActive = false;
        gPhase = BossIntroPhase::None;

        gTimer = 0.0f;
        gTextAlpha = 0.0f;
        gEyeAlpha = 0.0f;

        if (gPurpleParticles)
        {
            gPurpleParticles->SetSpawnRate(0.0f);
            gPurpleParticles->ReleaseAll();
        }
    }

    bool IsActive()
    {
        return gActive;
    }

    void Update(float dt)
    {
        if (!gActive)
            return;

        gTimer += dt;
        UpdatePurpleParticles();

        // default every frame
        gEyeAlpha = 0.0f;

        switch (gPhase)
        {
        case BossIntroPhase::Black:
        {
            if (gTimer >= BLACK_DURATION)
            {
                gPhase = BossIntroPhase::TextFadeIn;
                gTimer = 0.0f;
            }
        }
        break;

        case BossIntroPhase::TextFadeIn:
        {
            gTextAlpha = AEClamp(gTimer / TEXT_FADE_DURATION, 0.0f, 1.0f);

            if (gTimer >= TEXT_FADE_DURATION)
            {
                gTextAlpha = 1.0f;
                gPhase = BossIntroPhase::TextHold;
                gTimer = 0.0f;
            }
        }
        break;

        case BossIntroPhase::TextHold:
        {
            const float flashStart = 0.18f;
            const float flashEnd = 0.38f;

            if (gTimer >= flashStart && gTimer <= flashEnd)
            {
                float t = (gTimer - flashStart) / (flashEnd - flashStart);
                if (t < 0.5f)
                    gEyeAlpha = t / 0.5f;
                else
                    gEyeAlpha = (1.0f - t) / 0.5f;
            }

            if (gTimer >= TEXT_HOLD_DURATION)
            {
                gPhase = BossIntroPhase::EyelidOpen;
                gTimer = 0.0f;
            }
        }
        break;

        case BossIntroPhase::EyelidOpen:
        {
            UpdateEyelidOpen(dt);

            float t = AEClamp(gTimer / EYELID_OPEN_DURATION_HINT, 0.0f, 1.0f);
            gTextAlpha = 1.0f - t;

            if (EyelidFullyOpen())
            {
                gActive = false;
                gPhase = BossIntroPhase::None;
                gTimer = 0.0f;
                gTextAlpha = 0.0f;
                gEyeAlpha = 0.0f;

                if (gPurpleParticles)
                {
                    gPurpleParticles->SetSpawnRate(0.0f);
                    gPurpleParticles->ReleaseAll();
                }
            }
        }
        break;

        default:
            break;
        }
    }

    void Render()
    {
        if (!gActive)
            return;

        DrawEyelid();
        RenderPurpleParticles();
        RenderEyeFlash();
        RenderIntroText();
    }
}