#include "../Game/Background.h"
#include "../Utils/MeshGenerator.h"
#include "../Game/Camera.h"


void Background::Init() {
	rectMesh = MeshGenerator::GetRectMesh(1.0f, 1.0f);
	backgroundLayers[0] = AEGfxTextureLoad("Assets/Background.png");
	backgroundLayers[1] = AEGfxTextureLoad("Assets/Midground.png");
	backgroundLayers[2] = AEGfxTextureLoad("Assets/Foreground.png");
}
void Background::Render() {
	// Render state
	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
	AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
	AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetTransparency(1.0f);
	// ROTATION
	AEMtx33 rotate{ 0 };
	AEMtx33Identity(&rotate);

	// SCALE
	AEMtx33 scale;
	f32 scaleX = FOREGROUND_WIDTH;
	f32 scaleY = BACKGROUND_HEIGHT;
	AEMtx33Scale(&scale, scaleX, scaleY);

	// TRANSLATION
	AEMtx33 translate;
	f32 offsetX{ 0.f }, offsetY{ 0 };
	AEMtx33Trans(&translate,
		Camera::position.x * Camera::scale + offsetX,
		Camera::position.y * Camera::scale + offsetY);

	AEMtx33 transform;
	AEMtx33Concat(&transform, &rotate, &scale);
	AEMtx33Concat(&transform, &translate, &transform);

	AEGfxTextureSet(backgroundLayers[2], 0, 0);
	AEGfxSetTransform(transform.m);
	AEGfxMeshDraw(rectMesh, AE_GFX_MDM_TRIANGLES);
}
