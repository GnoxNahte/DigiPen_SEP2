#pragma once
#include "AEEngine.h"

class MeshGenerator
{
public:
	/**
	 * @brief		 Generate a rectangle mesh. Pivot is in the center
	 * @param width  Width of rectangle
	 * @param height Rectangle height
	 * @param color	 Vertex color
	 * @return		 Mesh pointer
	 */
	static AEGfxVertexList* GetRectMesh(float width, float height, u32 color);
	
	/**
	 * @brief		Generate a square mesh. Pivot is in the center
	 * @param width Width and height of rectangle
	 * @param color	Vertex color
	 * @return		Mesh pointer
	 */
	static AEGfxVertexList* GetSquareMesh(float width, u32 color);

	/**
	 * @brief		Generate a square mesh. Pivot is in the center
	 *				Might be better to just create 1 circle mesh and store scale in transform?
	 *				Unless need different vertex count at different size
	 * @param width Width and height of rectangle
	 * @param color	Vertex color
	 * @return		Mesh pointer
	 */
	static AEGfxVertexList* GetCircleMesh(float radius, u32 color, int vertexCount = 32);

	// Disable creating an instance. Static class
	MeshGenerator() = delete;
};
