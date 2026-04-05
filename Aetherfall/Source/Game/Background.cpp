/*!
@file		Background.cpp
@author 	Wei Xiang NG
@brief		This C++ file handles the background rendering of the game, including 
            loading background textures, implementing parallax scrolling based on 
            camera movement, and rendering multiple layers of the background with 
            appropriate scaling and tinting to create depth and atmosphere.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
#include "../Game/Background.h"
#include "../Utils/MeshGenerator.h"
#include "../Game/Camera.h"

/*-----------------------------------------------------------------------------
This function initializes the background by loading the necessary textures 
for each layer and creating a rectangular mesh that will be used to render 
the background layers. 

The textures are loaded from the specified file paths, and the mesh is 
generated to be a simple rectangle that can be scaled and transformed to fit 
the background layers. 

This setup allows for efficient rendering of the background with parallax 
scrolling, as the same mesh can be reused for each layer with different 
transformations and textures.
-----------------------------------------------------------------------------*/
void Background::Init() {
	rectMesh = MeshGenerator::GetRectMesh(1.0f, 1.0f);
	backgroundLayers[0] = AEGfxTextureLoad("Assets/Art/Background.png");
	backgroundLayers[1] = AEGfxTextureLoad("Assets/Art/Midground.png");
	backgroundLayers[2] = AEGfxTextureLoad("Assets/Art/Foreground.png");
}
/*-----------------------------------------------------------------------------
This function renders the background layers with parallax scrolling based on 
the camera's position.
-----------------------------------------------------------------------------*/
void Background::Render()
{
    // Basic render state
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    float tintStrength = 0.4f; // 0.0 is no tint, 1.0 is full blue

    // Mix the colors: (Target * strength) + (White * (1 - strength))
    float r = (0.196f * tintStrength) + (1.0f * (1.0f - tintStrength));
    float g = (0.278f * tintStrength) + (1.0f * (1.0f - tintStrength));
    float b = (0.549f * tintStrength) + (1.0f * (1.0f - tintStrength));

    AEGfxSetColorToMultiply(r, g, b, 0.85f);
    AEGfxSetColorToAdd(0,0,0,0);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.f);

    const f32 widths[3] = { (f32)BACKGROUND_WIDTH, (f32)MIDGROUND_WIDTH, (f32)FOREGROUND_WIDTH };
    //f32 winW = (f32)AEGfxGetWindowWidth();
    f32 winH = (f32)AEGfxGetWindowHeight();

    // Match texture height to screen height (or 1.2x as you had it)
    f32 targetHeight = winH * 1.2f;
    f32 scaleFactor = targetHeight / (f32)BACKGROUND_HEIGHT;

    // Adjust this to move the background down
    float globalYOffset = -360.f;

    for (int i = 0; i < UNIQUE_BG_TEXTURES; ++i)
    {
        float scaledWidth = widths[i] * scaleFactor;

        // PARALLAX CALCULATION
        float offset = Camera::position.x * Camera::scale * parallaxFactors[i];

        // Wrap the offset to tile infinitely
        float wrappedX = fmodf(offset, scaledWidth);
        if (wrappedX < 0) wrappedX += scaledWidth;

        // POSITIONING
        float startX = Camera::position.x * Camera::scale - wrappedX;

        // Account for Bottom-Left Origin + Centered Mesh
        startX += (scaledWidth / 2.0f);

        // Stay exactly with Camera Y-coordinates
        float drawY = Camera::position.y * Camera::scale + (winH / 2.0f) + globalYOffset;

        for (int x = -1; x <= 1; ++x)
        {
            float finalX = startX + (x * scaledWidth);

            AEMtx33 scale, trans, transform;
            AEMtx33Scale(&scale, scaledWidth, targetHeight);
            AEMtx33Trans(&trans, finalX, drawY);
            AEMtx33Concat(&transform, &trans, &scale);

            AEGfxTextureSet(backgroundLayers[i], 0, 0);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(rectMesh, AE_GFX_MDM_TRIANGLES);
        }
    }
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
}
/*-----------------------------------------------------------------------------
This function cleans up the background resources by freeing the mesh and 
unloading the textures used for the background layers. 

It ensures that all allocated resources are properly released to prevent 
memory leaks when the background is no longer needed, such as when 
transitioning to another scene or exiting the game.
-----------------------------------------------------------------------------*/
void Background::Exit() {
    if (rectMesh) {
        AEGfxMeshFree(rectMesh);
    }
    for (auto& tex : backgroundLayers)
    {
        if (tex)
        {
            AEGfxTextureUnload(tex);
            tex = nullptr;
        }
    }
}