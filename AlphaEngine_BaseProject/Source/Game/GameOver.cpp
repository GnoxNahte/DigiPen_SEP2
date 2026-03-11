#include "GameOver.h"
#include "AEEngine.h"
#include "../Game/Camera.h"

// Number of discrete animation frames to prebake
static const int   EYELID_FRAMES = 60;
static const int   SEGMENTS = 20;
static const float CURVE_STRENGTH = 0.000004f;
static const float CURVE_SCALE = 200.0f;
static const u32   BLACK = 0xFF000000;

static float eyelidProgress = 0.0f;
static float eyelidSpeed = 650.0f;

// Two arrays — one per eyelid, one entry per frame
static AEGfxVertexList* topFrames[EYELID_FRAMES] = {};
static AEGfxVertexList* bottomFrames[EYELID_FRAMES] = {};

static float ComputeCurve(float px)
{
    return -(CURVE_STRENGTH * px * px);
}

static AEGfxVertexList* BuildTopFrame(float progressNorm, float winW, float winH)
{
    float segmentWidth = 1.0f / SEGMENTS;
    AEGfxMeshStart();
    for (int i = 0; i < SEGMENTS; i++)
    {
        float x1 = -0.5f + i * segmentWidth;
        float x2 = -0.5f + (i + 1) * segmentWidth;

        float y1 = 0.5f - progressNorm + ComputeCurve(x1 * winW) * CURVE_SCALE / winH;
        float y2 = 0.5f - progressNorm + ComputeCurve(x2 * winW) * CURVE_SCALE / winH;
        if (y1 < 0.0f) y1 = 0.0f;
        if (y2 < 0.0f) y2 = 0.0f;

        AEGfxTriAdd(x1, 0.5f, BLACK, 0, 0,
            x2, 0.5f, BLACK, 0, 0,
            x2, y2, BLACK, 0, 0);
        AEGfxTriAdd(x1, 0.5f, BLACK, 0, 0,
            x2, y2, BLACK, 0, 0,
            x1, y1, BLACK, 0, 0);
    }
    return AEGfxMeshEnd();
}

static AEGfxVertexList* BuildBottomFrame(float progressNorm, float winW, float winH)
{
    float segmentWidth = 1.0f / SEGMENTS;
    AEGfxMeshStart();
    for (int i = 0; i < SEGMENTS; i++)
    {
        float x1 = -0.5f + i * segmentWidth;
        float x2 = -0.5f + (i + 1) * segmentWidth;

        float by1 = -0.5f + progressNorm - ComputeCurve(x1 * winW) * CURVE_SCALE / winH;
        float by2 = -0.5f + progressNorm - ComputeCurve(x2 * winW) * CURVE_SCALE / winH;
        if (by1 > 0.0f) by1 = 0.0f;
        if (by2 > 0.0f) by2 = 0.0f;

        AEGfxTriAdd(x1, -0.5f, BLACK, 0, 0,
            x2, -0.5f, BLACK, 0, 0,
            x2, by2, BLACK, 0, 0);
        AEGfxTriAdd(x1, -0.5f, BLACK, 0, 0,
            x2, by2, BLACK, 0, 0,
            x1, by1, BLACK, 0, 0);
    }
    return AEGfxMeshEnd();
}

// Call once in UI::Init()
void BuildEyelidMeshes()
{
    float winW = static_cast<float>(AEGfxGetWindowWidth());
    float winH = static_cast<float>(AEGfxGetWindowHeight());

    for (int i = 0; i < EYELID_FRAMES; i++)
    {
        // frame 0 = fully open (progress = 0), frame EYELID_FRAMES-1 = fully closed (progress = 0.5)
        float progressNorm = (i / static_cast<float>(EYELID_FRAMES - 1)) * 0.5f;

        topFrames[i] = BuildTopFrame(progressNorm, winW, winH);
        bottomFrames[i] = BuildBottomFrame(progressNorm, winW, winH);
    }
}

// Call once in UI::Exit()
void FreeEyelidMeshes()
{
    for (int i = 0; i < EYELID_FRAMES; i++)
    {
        if (topFrames[i]) { AEGfxMeshFree(topFrames[i]);    topFrames[i] = nullptr; }
        if (bottomFrames[i]) { AEGfxMeshFree(bottomFrames[i]); bottomFrames[i] = nullptr; }
    }
}

// Call in UI::Update() when player is dead
void UpdateEyelid(float dt)
{
    float halfH = AEGfxGetWindowHeight() * 0.5f;
    float remaining = halfH - eyelidProgress;
    float speed = remaining * 1.35f; // proportional to remaining distance
    speed = AEClamp(speed, 80.0f, eyelidSpeed); // floor prevents infinite crawl
    eyelidProgress += speed * dt;
    if (eyelidProgress > halfH)
        eyelidProgress = halfH;
}

// Call in UI::Render() when player is dead
void DrawEyelid()
{
    if (eyelidProgress <= 0.0f) return;

    float winW = static_cast<float>(AEGfxGetWindowWidth());
    float winH = static_cast<float>(AEGfxGetWindowHeight());

    // Map progress to frame index — same rounding pattern as cooldown meter
    float progressNorm = eyelidProgress / winH; // 0.0 -> 0.5
    float t = progressNorm / 0.5f;              // 0.0 -> 1.0
    int frameIndex = static_cast<int>(t * (EYELID_FRAMES - 1) + 0.5f);
    //int frameIndex = static_cast<int>((1.0f - t) * (EYELID_FRAMES - 1) + 0.5f);
    if (frameIndex < 0)                frameIndex = 0;
    if (frameIndex >= EYELID_FRAMES)   frameIndex = EYELID_FRAMES - 1;

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_NONE);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1, 1, 1, 1);
    AEGfxSetColorToAdd(0, 0, 0, 0);

    // Match DrawHealthVignette transform exactly
    AEMtx33 scale, translate, transform;
    AEMtx33Scale(&scale, winW, winH);
    AEMtx33Trans(&translate,
        Camera::position.x * Camera::scale,
        Camera::position.y * Camera::scale);
    AEMtx33Concat(&transform, &translate, &scale);

    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(topFrames[frameIndex], AE_GFX_MDM_TRIANGLES);
    AEGfxMeshDraw(bottomFrames[frameIndex], AE_GFX_MDM_TRIANGLES);
}

bool EyelidDone()
{
    return eyelidProgress >= AEGfxGetWindowHeight() * 0.5f;
}

// Resets progress only — meshes untouched
void ResetEyelid()
{
    eyelidProgress = 0.0f;
}
float GetEyelidProgress()
{
    return eyelidProgress;
}
