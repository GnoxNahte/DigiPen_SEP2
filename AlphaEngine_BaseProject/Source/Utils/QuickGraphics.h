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
	static void DrawRect(const AEVec2& position, const AEVec2& scale, u32 color = 0xFFFFFFFF);
	static void DrawRect(float posX, float posY, float scaleX, float scaleY, u32 color = 0xFFFFFFFF);

	// u32 variant
	static void SetColorToMultiply(u32 color);
private:
	static AEGfxVertexList* rect;

	// Disable creating an instance. Static class
	QuickGraphics() = delete;
};

