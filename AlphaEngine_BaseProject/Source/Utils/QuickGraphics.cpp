#include "QuickGraphics.h"
#include "../Game/Camera.h"

AEGfxVertexList* QuickGraphics::rect = nullptr;
s8 QuickGraphics::font = 0;

void QuickGraphics::Init()
{
    rect = MeshGenerator::GetSquareMesh(1.f);
    font = AEGfxCreateFont("Assets/liberation-mono.ttf", 72);
}

void QuickGraphics::Free()
{
    AEGfxMeshFree(rect);
    AEGfxDestroyFont(font);
}

void QuickGraphics::DrawRect(float posX, float posY, float scaleX, float scaleY, u32 color, AEGfxMeshDrawMode drawMode)
{
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    SetColorToMultiply(color);

    AEMtx33 transform;
    AEMtx33Scale(&transform, scaleX, scaleY); // local scale
    AEMtx33TransApply(&transform, &transform, posX, posY); // local transform
    AEMtx33ScaleApply(&transform, &transform, Camera::scale, Camera::scale); // local -> world space
    AEGfxSetTransform(transform.m);

    AEGfxMeshDraw(rect, drawMode);
    
    // Reset
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
}

void QuickGraphics::PrintText(const char* str, f32 x, f32 y, f32 scale, f32 red, f32 green, f32 blue, f32 alpha)
{
    AEGfxPrint(font, str, x, y, scale, red, green, blue, alpha);
}

void QuickGraphics::DrawRect(const AEVec2& position, const AEVec2& scale, u32 color, AEGfxMeshDrawMode drawMode)
{
    DrawRect(position.x, position.y, scale.x, scale.y, color, drawMode);
}

void QuickGraphics::SetColorToMultiply(u32 color)
{
    AEGfxSetColorToMultiply(
        (float)((color >> 16) & 0xFF), // Red
        (float)((color >> 8) & 0xFF), // Green
        (float)((color >> 0) & 0xFF), // Blue
        (float)((color >> 24) & 0xFF)// Alpha
    );
}
