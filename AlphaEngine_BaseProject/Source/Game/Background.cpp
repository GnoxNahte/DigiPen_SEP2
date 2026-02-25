#include "../Game/Background.h"
#include "../Utils/MeshGenerator.h"
#include "../Game/Camera.h"


void Background::Init() {
	rectMesh = MeshGenerator::GetRectMesh(1.0f, 1.0f);
	backgroundLayers[0] = AEGfxTextureLoad("Assets/Background.png");
	backgroundLayers[1] = AEGfxTextureLoad("Assets/Midground.png");
	backgroundLayers[2] = AEGfxTextureLoad("Assets/Foreground.png");
}
void Background::Render()
{
    // Basic render state
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    float tintStrength = 0.4f; // 0.0 is no tint, 1.0 is full blue

    // Mix the colors: (Target * strength) + (White * (1 - strength))
    float r = (0.196f * tintStrength) + (1.0f * (1.0f - tintStrength));
    float g = (0.278f * tintStrength) + (1.0f * (1.0f - tintStrength));
    float b = (0.549f * tintStrength) + (1.0f * (1.0f - tintStrength));

    AEGfxSetColorToMultiply(r, g, b, 1.0f);
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
    float globalYOffset = -100.0f;

    for (int i = 0; i < UNIQUE_BG_TEXTURES; ++i)
    {
        float scaledWidth = widths[i] * scaleFactor;

        // PARALLAX CALCULATION
        // We calculate how much the texture has "slid" based on camera movement
        // If factor is 0, offset is 0.
        float offset = Camera::position.x * Camera::scale * parallaxFactors[i];

        // Wrap the offset so we can tile infinitely
        float wrappedX = fmodf(offset, scaledWidth);
        if (wrappedX < 0) wrappedX += scaledWidth;

        // POSITIONING
        // We start at the camera's current world position.
        // We subtract the wrappedX to create the "scrolling" effect relative to the camera.
        float startX = Camera::position.x * Camera::scale - wrappedX;

        // Account for Bottom-Left Origin + Centered Mesh
        // This puts the first tile exactly centered on the camera's view
        startX += (scaledWidth / 2.0f);

        // Vertical Lock: Must stay exactly with Camera Y
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