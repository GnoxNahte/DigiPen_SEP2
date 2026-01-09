#include "QuickGraphics.h"
#include "../Game/Camera.h"

AEGfxVertexList* QuickGraphics::rect = nullptr;

void QuickGraphics::Init()
{
    rect = MeshGenerator::GetSquareMesh(1.f);
}

void QuickGraphics::DrawRect(float posX, float posY, float scaleX, float scaleY, u32 color)
{
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    SetColorToMultiply(color);

    AEMtx33 transform;
    AEMtx33Trans(&transform, posX, posY);
    AEMtx33ScaleApply(&transform, &transform, Camera::scale * scaleX, Camera::scale * scaleY);
    AEGfxSetTransform(transform.m);

    AEGfxMeshDraw(rect, AE_GFX_MDM_TRIANGLES);

    // Reset
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
}

void QuickGraphics::DrawRect(const AEVec2& position, const AEVec2& scale, u32 color)
{
    DrawRect(position.x, position.y, scale.x, scale.y, color);
}

void QuickGraphics::SetColorToMultiply(u32 color)
{
    AEGfxSetColorToMultiply(
        (float)((color >> 16) & 0xFF), // Red
        (float)((color >>  8) & 0xFF), // Green
        (float)((color >>  0) & 0xFF), // Blue
        (float)((color >> 24) & 0xFF )// Alpha
    );
}
