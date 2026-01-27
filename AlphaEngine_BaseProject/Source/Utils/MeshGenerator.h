#pragma once
#include "AEEngine.h"

namespace MeshGenerator
{
	/**
	 * @brief		   Generate a rectangle mesh. Pivot is in the center. Bottom-left UV = (0, 0), Top-right UV = (uvWidth, uvHeight)
	 *				   For spritesheet
	 * @param width    Width of rectangle
	 * @param height   Rectangle height
	 * @param uvWidth  Right UV. Left UV = 0
	 * @param uvHeight Bottom UV. Top UV = 0
	 * @param color	   Vertex color
	 * @return		   Mesh pointer
	 */
	AEGfxVertexList* GetRectMesh(float width, float height, float uvWidth, float uvHeight, u32 color = 0xFFFFFFFF);
	
	/**
	 * @brief		 Generate a rectangle mesh. Pivot is in the center
	 * @param width  Width of rectangle
	 * @param height Rectangle height
	 * @param color	 Vertex color
	 * @return		 Mesh pointer
	 */
	AEGfxVertexList* GetRectMesh(float width, float height, u32 color = 0xFFFFFFFF);
	
	/**
	 * @brief		Generate a square mesh. Pivot is in the center
	 * @param width Width and height of rectangle
	 * @param color	Vertex color
	 * @return		Mesh pointer
	 */
	AEGfxVertexList* GetSquareMesh(float width, u32 color = 0xFFFFFFFF);
	
	/**
	 * @brief		Generate a square mesh. Pivot is in the center
	 * @param width Width and height of rectangle
	 * @param uvWidth  Right UV. Left UV = 0
	 * @param uvHeight Bottom UV. Top UV = 0
	 * @param color	Vertex color
	 * @return		Mesh pointer
	 */
	AEGfxVertexList* GetSquareMesh(float width, float uvWidth, float uvHeight, u32 color = 0xFFFFFFFF);

	/**
	 * @brief		Generate a square mesh. Pivot is in the center
	 *				Might be better to just create 1 circle mesh and store scale in transform?
	 *				Unless need different vertex count at different size
	 * @param radius		Radius of circle
	 * @param color			Vertex color
	 * @param vertexCount	Number of vertices on the outside the circle has (not counting the center)
	 * @return				Mesh pointer
	 */
	AEGfxVertexList* GetCircleMesh(float radius, u32 color = 0xFFFFFFFF, int vertexCount = 32);
};
