#include "MeshGenerator.h"

AEGfxVertexList* GetRectMesh(f32 width, f32 height, u32 color)
{
	AEGfxMeshStart();

	// Half width
	f32 hw = width * 0.5f;
	f32 hh = height * 0.5f;

	AEGfxTriAdd(
		-hw, -hh, color, 0.0f, 1.0f, // Bottom left
		hw, -hh, color, 1.0f, 1.0f, // Bottom right
		-hw, hh, color, 0.0f, 0.0f  // Top left
	);

	AEGfxTriAdd(
		hw, -hh, color, 1.0f, 1.0f, // Bottom right
		hw, hh, color, 1.0f, 0.0f, // Top right
		-hw, hh, color, 0.0f, 0.0f  // Top left
	);

	return AEGfxMeshEnd();
}

AEGfxVertexList* GetCircleMesh(f32 radius, u32 color, int vertexCount)
{
	if (vertexCount < 3)
	{
		printf("[WARNING] Circle vertex count < 3. Setting to 3");
		vertexCount = 3;
	}

	AEGfxMeshStart();

	//float prevX = 0.f, prevY = 1.f;
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