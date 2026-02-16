#include "BuffCards.h"
#include "../Utils/MeshGenerator.h"

void BuffCards::Init() {
	cardMesh = MeshGenerator::GetRectMesh(1.0f, 1.0f);
	cardBackTex = AEGfxTextureLoad("Assets/0_CardBack.png");
}
void BuffCards::Render() {
	// Create a scale matrix that scales by scaling factor.
	AEMtx33 scale = { 0 };
	const f32 CARD_SIZE_MODIFIER = 0.35f;
	AEMtx33Scale(&scale, CARD_WIDTH * CARD_SIZE_MODIFIER, CARD_HEIGHT * CARD_SIZE_MODIFIER);

	// Create a rotation matrix, base card doesn't rotate.
	AEMtx33 rotate = { 0 };
	AEMtx33Rot(&rotate, 0);

	// Create a translation matrix that translates by
	// 200 in the x-axis and 100 in the y-axis
	AEMtx33 translate = { 0 };
	AEMtx33Trans(&translate, (AEGfxGetWinMaxX() / 2), (AEGfxGetWinMaxY() / 2));

	// Concatenate the matrices into the 'transform' variable.
	AEMtx33 transform = { 0 };
	AEMtx33Concat(&transform, &rotate, &scale);
	AEMtx33Concat(&transform, &translate, &transform);

	// Draw something with texture.
	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);

	// Set the the color to multiply to white, so that the sprite can 
	// display the full range of colors (default is black).
	AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

	// Set the color to add to nothing, so that we don't alter the sprite's color
	AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

	// Set blend mode to AE_GFX_BM_BLEND, which will allow transparency.
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetTransparency(1.0f);

	// Tell Alpha Engine to use the texture stored in pTex
	AEGfxTextureSet(cardBackTex, 0, 0);

	// Tell Alpha Engine to use the matrix in 'transform' to apply onto all 
	// the vertices of the mesh that we are about to choose to draw in the next line.
	AEGfxSetTransform(transform.m);

	// Tell Alpha Engine to draw the mesh with the above settings.
	AEGfxMeshDraw(cardMesh, AE_GFX_MDM_TRIANGLES);
}
void BuffCards::Exit() {
	if (cardMesh) {
		// Free card meshes.
		AEGfxMeshFree(cardMesh);
	}
	if (cardBackTex) {
		// Free card textures.
		AEGfxTextureUnload(cardBackTex);
	}
}