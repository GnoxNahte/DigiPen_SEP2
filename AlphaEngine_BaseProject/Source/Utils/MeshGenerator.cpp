#include "MeshGenerator.h"

AEGfxVertexList* MeshGenerator::GetRectMesh(float width, float height, float uvWidth, float uvHeight, u32 color)
{
	AEGfxMeshStart();

	// Half width
	float hw = width * 0.5f;
	float hh = height * 0.5f;

	AEGfxTriAdd(
		-hw, -hh, color, 0.0f, uvHeight, // Bottom left
		 hw, -hh, color, uvWidth, uvHeight, // Bottom right
		-hw,  hh, color, 0.0f, 0.0f  // Top left
	);

	AEGfxTriAdd(
		 hw, -hh, color, uvWidth, uvHeight, // Bottom right
		 hw,  hh, color, uvWidth, 0.f, // Top right
		-hw,  hh, color, 0.0f, 0.0f  // Top left
	);

	return AEGfxMeshEnd();
}

AEGfxVertexList* MeshGenerator::GetRectMesh(float width, float height, u32 color)
{
	return GetRectMesh(width, height, 1.0f, 1.0f, color);
}

AEGfxVertexList* MeshGenerator::GetSquareMesh(float width, u32 color)
{
	return GetRectMesh(width, width, 1.0f, 1.0f, color);
}

AEGfxVertexList* MeshGenerator::GetSquareMesh(float width, float uvWidth, float uvHeight, u32 color)
{
	return GetRectMesh(width, width, uvWidth, uvHeight, color);
}

AEGfxVertexList* MeshGenerator::GetCircleMesh(float radius, u32 color, int vertexCount)
{
	if (vertexCount < 3)
	{
		printf("[WARNING] Circle vertex count < 3. Setting to 3");
		vertexCount = 3;
	}

	AEGfxMeshStart();

	float angleDiff = 2 * PI / vertexCount;
	float currAngle = 0;

	float prevX = 0.f, prevY = 1.f;

	for (int i = 0; i < vertexCount; i++)
	{
		float nextX = sinf(currAngle + angleDiff);
		float nextY = cosf(currAngle + angleDiff);

		AEGfxTriAdd(
			0.f, 0.f, color, 0.5f, 0.5f, // Center
			prevX * radius, prevY * radius, color, 1.0f, 0.0f, // Current angle
			nextX * radius, nextY * radius, color, 0.0f, 0.0f  // Next angle
		);

		prevX = nextX;
		prevY = nextY;

		currAngle += angleDiff;
	}

	return AEGfxMeshEnd();
}
