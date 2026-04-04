/*!
@file	QuickGraphics.h
@author	Ethan Ong
@brief	Declares QuickGraphics which makes it easier to draw simple shapes to the screen

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/

#pragma once
#include "MeshGenerator.h"
#include "AEEngine.h"

class QuickGraphics
{
public:
	/**
	 * @brief	Create the meshes.
	 *			Need to do here because AlphaEngine throws exception if
	 *			creating the mesh directly.
	 *			Might be because AlphaEngine hasn't been initialised
	 */
	static void Init();
	static void Free();
	static void DrawRect(const AEVec2& position, const AEVec2& scale, u32 color = 0xFFFFFFFF, AEGfxMeshDrawMode drawMode = AE_GFX_MDM_TRIANGLES);
	static void DrawRect(float posX, float posY, float scaleX, float scaleY, u32 color = 0xFFFFFFFF, AEGfxMeshDrawMode drawMode = AE_GFX_MDM_TRIANGLES);

	static void DrawRay(const AEVec2& start, const AEVec2& end, float width = 0.1f, u32 color = 0xFFFFFFFF);

	// Duplicate of AEGfxPrint but without fontId
	static void PrintText(const char* str, f32 x, f32 y, f32 scale, f32 red, f32 green, f32 blue, f32 alpha);

	// Print text in world-space
	static void PrintWorldText(const char* str, f32 x, f32 y, f32 scale, f32 red, f32 green, f32 blue, f32 alpha);

	// u32 variant
	static void SetColorToMultiply(u32 color);
private:
	static AEGfxVertexList* rect;
	static s8 font;

	// Disable creating an instance. Static class
	QuickGraphics() = delete;
};

