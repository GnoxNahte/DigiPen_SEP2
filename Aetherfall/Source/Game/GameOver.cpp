/*!
@file		GameOver.cpp
@author 	Wei Xiang NG
@brief		This C++ file implements the game over screen effect, which simulates 
            the player's vision closing in as they die. It pre-bakes a series of 
            meshes representing different stages of the eyelids closing, and then 
            renders the appropriate mesh based on the player's death progress. The 
            effect is designed to be visually impactful while being efficient by 
            using precomputed meshes instead of real-time calculations.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
#include "GameOver.h"
#include "../Game/Camera.h"

// Number of discrete animation frames to prebake
static const int   EYELID_FRAMES = 60;
static const int   SEGMENTS = 20;
static const float CURVE_STRENGTH = 0.000004f;
static const float CURVE_SCALE = 200.0f;
static const u32   BLACK = 0xFF000000;

static float eyelidProgress = 0.0f;
static float eyelidSpeed = 650.0f;

// Two arrays, one per eyelid, one entry per frame
static AEGfxVertexList* topFrames[EYELID_FRAMES] = {};
static AEGfxVertexList* bottomFrames[EYELID_FRAMES] = {};

/*-----------------------------------------------------------------------------
This function computes the vertical offset for a given horizontal position (px) 
based on a quadratic curve. The curve is designed to create a bulging effect 
in the middle of the screen, which simulates the natural shape of eyelids as 
they close. 

The strength of the curve is determined by the CURVE_STRENGTH 
constant, and the resulting offset is negative to create a bulge upwards for 
the top eyelid and downwards for the bottom eyelid. 

This function is used when building the meshes for the eyelids to add a dynamic 
curvature to the closing effect, making it more visually appealing and 
less linear.
-----------------------------------------------------------------------------*/
static float ComputeCurve(float px)
{
    return -(CURVE_STRENGTH * px * px);
}
/*-----------------------------------------------------------------------------
This function builds the vertex list for the top eyelid frame based on the given
progress of the closing animation (progressNorm) and the window dimensions 
(winW, winH).

The function divides the width of the screen into segments and calculates the
vertical position of the eyelid for each segment based on the progress and the
curvature computed by the ComputeCurve function. 

It then creates triangles to form the shape of the eyelid, ensuring that it 
does not go below the center of the screen.
-----------------------------------------------------------------------------*/
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
/*-----------------------------------------------------------------------------
This function builds the vertex list for the bottom eyelid frame based on the 
given progress of the closing animation (progressNorm) and the window dimensions
(winW, winH). 

It follows a similar approach to BuildTopFrame, dividing the width into segments
and calculating the vertical position of the eyelid for each segment. 

The main difference is that the bottom eyelid moves upwards as it closes, so the
progress is added to the base position instead of subtracted, and the curvature
creates a bulge downwards. 

The function also ensures that the bottom eyelid does not go above the center 
of the screen.
-----------------------------------------------------------------------------*/
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

/*-----------------------------------------------------------------------------
This function builds the meshes for all frames of the eyelid closing animation. 
It calculates the appropriate progress for each frame and calls the BuildTopFrame 
and BuildBottomFrame functions to generate the vertex lists for the top and bottom
eyelids.

Callers should only need to call this once during initialization, as the meshes 
are precomputed and stored in the topFrames and bottomFrames arrays for 
efficient rendering during the game over sequence.
-----------------------------------------------------------------------------*/
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

/*-----------------------------------------------------------------------------
This function frees the meshes for all frames of the eyelid closing animation. 

It iterates through the topFrames and bottomFrames arrays and calls AEGfxMeshFree 
on each mesh, then sets the pointers to nullptr to avoid dangling references.

Callers should call this during shutdown to clean up resources used by the 
eyelid effect and prevent memory leaks.
-----------------------------------------------------------------------------*/
void FreeEyelidMeshes()
{
    for (int i = 0; i < EYELID_FRAMES; i++)
    {
        if (topFrames[i]) { AEGfxMeshFree(topFrames[i]);    topFrames[i] = nullptr; }
        if (bottomFrames[i]) { AEGfxMeshFree(bottomFrames[i]); bottomFrames[i] = nullptr; }
    }
}

/*-----------------------------------------------------------------------------
This function draws the eyelid effect on the screen based on the current 
progress of the closing animation (eyelidProgress). 

It calculates the appropriate frame index to render based on the progress and 
then draws the corresponding meshes for the top and bottom eyelids. 

The function also sets the appropriate render state for drawing the eyelids, 
including disabling blending and setting the color to black. The transform is 
set to scale the meshes to cover the entire screen and to follow the camera's 
position and scale, ensuring that the eyelid effect remains centered and 
properly sized regardless of camera movement or zoom level.

Callers should call this during the rendering phase of the game over sequence 
to display the closing eyelid effect as the player dies.
-----------------------------------------------------------------------------*/
void DrawEyelid()
{
    if (eyelidProgress <= 0.0f) return;

    float winW = static_cast<float>(AEGfxGetWindowWidth());
    float winH = static_cast<float>(AEGfxGetWindowHeight());

    // Map progress to frame index ?same rounding pattern as cooldown meter
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

/*-----------------------------------------------------------------------------
This function resets the eyelid progress to 0, which corresponds to the 
fully open state.
-----------------------------------------------------------------------------*/
void ResetEyelid()
{
    eyelidProgress = 0.0f;
}
/*-----------------------------------------------------------------------------
This function returns the current progress of the eyelid closing animation in 
pixels.
-----------------------------------------------------------------------------*/
float GetEyelidProgress()
{
    return eyelidProgress;
}
/*-----------------------------------------------------------------------------
This function returns the maximum progress value for the eyelid closing 
animation in pixels, which corresponds to the fully closed state. 

It is calculated as half of the window height, since the eyelids meet in the
middle of the screen when fully closed.
-----------------------------------------------------------------------------*/
float GetEyelidMaxProgress()
{
    return AEGfxGetWindowHeight() * 0.5f;
}
/*-----------------------------------------------------------------------------
This function sets the progress of the eyelid closing animation in pixels. 

It clamps the input value to ensure it stays within the valid range of 0 
(fully open) to the maximum progress (fully closed).
-----------------------------------------------------------------------------*/
void SetEyelidProgress(float progressPx)
{
    float maxP = GetEyelidMaxProgress();
    if (progressPx < 0.0f) progressPx = 0.0f;
    if (progressPx > maxP) progressPx = maxP;
    eyelidProgress = progressPx;
}
/*-----------------------------------------------------------------------------
This function updates the progress of the eyelid closing animation based on 
the elapsed time (dt).
-----------------------------------------------------------------------------*/
void UpdateEyelidClose(float dt)
{
    float halfH = GetEyelidMaxProgress();
    float remaining = halfH - eyelidProgress;
    float speed = remaining * 1.35f;
    speed = AEClamp(speed, 80.0f, eyelidSpeed);

    eyelidProgress += speed * dt;
    if (eyelidProgress > halfH)
        eyelidProgress = halfH;
}
/*-----------------------------------------------------------------------------
This function updates the progress of the eyelid opening animation based on the 
elapsed time (dt). It calculates the speed of opening based on the current 
progress, ensuring that it opens faster when more closed and slows down as 
it approaches fully open. 

The speed is clamped to a minimum value to ensure it doesn't take too long to 
open from a fully closed state, and it is also clamped to the eyelidSpeed to 
prevent it from opening too quickly.
-----------------------------------------------------------------------------*/
void UpdateEyelidOpen(float dt)
{
    float maxP = GetEyelidMaxProgress();
    float speed = maxP * 1.0f;
    speed = AEClamp(speed, 80.0f, eyelidSpeed);

    eyelidProgress -= speed * dt;
    if (eyelidProgress < 0.0f)
        eyelidProgress = 0.0f;
}
/*-----------------------------------------------------------------------------
This function draws the eyelid effect at a specific progress value in pixels.

It is similar to the DrawEyelid function but allows for drawing the eyelids at 
an arbitrary progress value, which can be useful for debugging or for special 
effects where the progress is not tied to the player's death animation.
-----------------------------------------------------------------------------*/
void DrawEyelidAtProgress(float progressPx)
{
    if (progressPx <= 0.0f) return;

    float winW = static_cast<float>(AEGfxGetWindowWidth());
    float winH = static_cast<float>(AEGfxGetWindowHeight());

    float progressNorm = progressPx / winH;
    float t = progressNorm / 0.5f;

    int frameIndex = static_cast<int>(t * (EYELID_FRAMES - 1) + 0.5f);
    if (frameIndex < 0) frameIndex = 0;
    if (frameIndex >= EYELID_FRAMES) frameIndex = EYELID_FRAMES - 1;

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_NONE);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1, 1, 1, 1);
    AEGfxSetColorToAdd(0, 0, 0, 0);

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
/*-----------------------------------------------------------------------------
This function checks if the eyelids are fully closed by comparing the current 
progress to half of the maximum progress, which corresponds to the point where 
the eyelids meet in the middle of the screen. 

If the progress is greater than or equal to half of the maximum progress, 
it returns true, indicating that the eyelids are fully closed. 
Otherwise, it returns false.
-----------------------------------------------------------------------------*/
bool EyelidFullyClosed()
{ 
    return eyelidProgress >= GetEyelidMaxProgress() * 0.5;
}
/*-----------------------------------------------------------------------------
This function checks if the eyelids are fully open by checking if the current 
progress is less than or equal to 0, which corresponds to the state where the 
eyelids are completely open and not covering any part of the screen.
-----------------------------------------------------------------------------*/
bool EyelidFullyOpen()
{
    return eyelidProgress <= 0.0f;
}
