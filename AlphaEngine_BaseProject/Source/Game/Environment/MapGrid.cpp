#include <iostream>

#include "MapGrid.h"
#include "../../Utils/MeshGenerator.h"
#include "../Camera.h"
#include "../../Utils/AEExtras.h"

MapGrid::MapGrid(int rows, int cols)
	:	size(rows, cols), 
		tiles(rows* cols)
{
	mesh = MeshGenerator::GetSquareMesh(1.f, 1.f / MapTile::typeCount, 1.f);
	tilemapTexture = AEGfxTextureLoad("Assets/Tmp/tmp-tilemap.png");

	tileCount = size.x * size.y;
	 
	// for (int i = 0; i < uvCoordsSize; ++i)
	// {
	// 	uvCoords[i].x = 1.f / uvCoordsSize;
	// 	uvCoords[i].y = 1.f;
	// }

	// === Just for testing ===
	for (int y = 0; y < size.y; y += 3)
	{
		for (int x = 5; x < size.x / 2; x++)
		{
			tiles[y * size.x + x].type = (x % 4 >= 2) ? MapTile::Type::GROUND : MapTile::Type::NONE;
		}
	}

	for (int y = 0; y < size.y; y += 4)
	{
		for (int x = size.x / 2; x < size.x - 5; x++)
		{
			tiles[y * size.x + x].type = (x % 4 >= 2) ? MapTile::Type::GROUND : MapTile::Type::NONE;
		}
	}

	// Set horizontal borders 
	for (int x = 0; x < size.x; ++x)
	{
		tiles[0				* size.x + x].type = MapTile::Type::GROUND;
		tiles[1				* size.x + x].type = MapTile::Type::GROUND;
		tiles[(size.y - 1)  * size.x + x].type = MapTile::Type::GROUND;
		tiles[(size.y - 2)  * size.x + x].type = MapTile::Type::GROUND;
	}

	// Set vertical borders 
	for (int y = 0; y < size.y; ++y)
	{
		tiles[y * size.x + 0].type = MapTile::Type::GROUND;
		tiles[y * size.x + 1].type = MapTile::Type::GROUND;
		//tiles[y * size.x + 5].type = MapTile::Type::GROUND;
		tiles[y * size.x + size.x - 1].type = MapTile::Type::GROUND;
		tiles[y * size.x + size.x - 2].type = MapTile::Type::GROUND;
	}
}

/* Calling it's own construct is tmp only. todo proper file read */
MapGrid::MapGrid(const char*) : MapGrid(10, 10)
{
}

MapGrid::~MapGrid()
{
	AEGfxMeshFree(mesh);
	AEGfxTextureUnload(tilemapTexture);
}

void MapGrid::Render(const Camera& camera)
{
	AEMtx33 transform;

	// Get corners of screen in world space
	AEVec2 topLeft, botRight;
	AEExtras::ScreenToWorldPosition(AEVec2(0, 0), camera.position, topLeft);
	AEExtras::ScreenToWorldPosition(AEVec2((f32)AEGfxGetWindowWidth(), (f32)AEGfxGetWindowHeight()), camera.position, botRight);

	// Offset 
	botRight.x += 1.f;
	botRight.y += 1.f;

	// Get grid coordinates for tiles that only the camera can see
	int minX, minY, maxX, maxY;
	WorldToGridCoordsClamped(topLeft, minX, minY);
	WorldToGridCoordsClamped(botRight, maxX, maxY);

	//std::cout << std::setw(3) << minX << std::setw(3) << minY << std::setw(3) << maxX << std::setw(3) << maxY << "   ";

	// Loop through each row, only showing tiles that the camera can see
	for (int y = minY; y < maxY; y++)
	{
		// Loop through each column only showing tiles that the camera can see
		for (int x = minX; x < maxX; x++)
		{
			const MapTile* tile = GetTile(x, y);
			if (tile == nullptr)
				continue;

			// Local scale. For flipping sprite's facing direction
			AEMtx33Trans(&transform, (float)(x + 0.5f), (float)(y + 0.5f));
			// Camera scale. Scales previous translation too.
			AEMtx33ScaleApply(&transform, &transform, Camera::scale, Camera::scale);
			AEGfxSetTransform(transform.m);

			AEGfxTextureSet(tilemapTexture, (int)tile->type * (1.f / MapTile::typeCount), 1.f);
			AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
		}
	}
}

inline const MapTile* MapGrid::GetTile(int x, int y)
{
#if _DEBUG
	if (x < 0 || x >= size.x || y < 0 || y >= size.y)
		return nullptr;
#endif
	return &(tiles[y * size.x + x]);
}
//level editor 
void MapGrid::SetTile(int x, int y, MapTile::Type type)
{
	if (x < 0 || x >= size.x || y < 0 || y >= size.y)
		return;

	tiles[y * size.x + x].type = type;
}
//level editor
bool MapGrid::CheckPointCollision(float x, float y)
{
	int index = WorldToIndex(x, y);
	if (index < 0)
		return false;

	return tiles[index].type != MapTile::Type::NONE;
}

bool MapGrid::CheckPointCollision(const AEVec2& worldPosition)
{
	return CheckPointCollision(worldPosition.x, worldPosition.y);
}

bool MapGrid::CheckBoxCollision(const AEVec2& boxPosition, const AEVec2& boxSize)
{
	AEVec2 halfBoxSize(boxSize.x * 0.5f, boxSize.y * 0.5f);

	// Check with 
	return	CheckPointCollision(boxPosition.x - halfBoxSize.x, boxPosition.y - halfBoxSize.y) ||
			CheckPointCollision(boxPosition.x - halfBoxSize.x, boxPosition.y + halfBoxSize.y) ||
			CheckPointCollision(boxPosition.x + halfBoxSize.x, boxPosition.y - halfBoxSize.y) ||
			CheckPointCollision(boxPosition.x + halfBoxSize.x, boxPosition.y + halfBoxSize.y);
}

bool MapGrid::CheckBoxCollision(const Box& box)
{
	return CheckBoxCollision(box.position, box.size);
}

void MapGrid::HandleBoxCollision(AEVec2& currentPosition, AEVec2& velocity, const AEVec2& nextPosition, const AEVec2& colliderSize)
{
	// This is a simple and quick collision handler. Should not use when (currentPosition - nextPosition).length > 1 tiles OR colliderSize > 1 tile
	// todo? proper line drawing (continuous collision)? https://www.redblobgames.com/grids/line-drawing/
	// tood? do proper collision? https://www.youtube.com/watch?v=8JJ-4JgR7Dg

	AEVec2 halfColliderSize(colliderSize.x * 0.5f, colliderSize.y * 0.5f);

	// ===== Check if in map =====
	/*if (currentPosition.x < 0)
		currentPosition.x = halfColliderSize.x;
	else if (currentPosition.x > size.x - halfColliderSize.x)
		currentPosition.x = size.x - halfColliderSize.x;

	if (currentPosition.y < 0)
		currentPosition.y = halfColliderSize.y;
	else if (currentPosition.y > size.y - halfColliderSize.y)
		currentPosition.y = size.y - halfColliderSize.y;*/

	// ===== Get top left & bottom right (from current and next position) =====
	AEVec2 bottomLeft, topRight;
	bottomLeft.x = min(currentPosition.x, nextPosition.x) - halfColliderSize.x,
	bottomLeft.y = min(currentPosition.y, nextPosition.y) - halfColliderSize.y,
	topRight.x = max(currentPosition.x, nextPosition.x) + halfColliderSize.x,
	topRight.y = max(currentPosition.y, nextPosition.y) + halfColliderSize.y;

	//AEVec2	topRight(topRight.x, bottomLeft.y),
	//	bottomLeft(bottomLeft.x, topRight.y);

	int nextGridX, nextGridY;
	WorldToGridCoords(nextPosition, nextGridX, nextGridY);

	bool isMovingRight = nextPosition.x > currentPosition.x;
	bool isMovingUp = nextPosition.y > currentPosition.y;

	float clearance = 0.2f;

	// Moving right
	if (isMovingRight)
	{
		float colliderPos = nextPosition.x + halfColliderSize.x;
		if (CheckPointCollision(colliderPos, topRight.y - clearance) || CheckPointCollision(colliderPos, bottomLeft.y + clearance))
		{
			currentPosition.x = nextGridX + 1.01f - halfColliderSize.x;
			velocity.x = 0.f;
		}
		else
			currentPosition.x = nextPosition.x;
	}
	// Moving left
	else
	{
		float colliderPos = nextPosition.x - halfColliderSize.x;
		if (CheckPointCollision(colliderPos, topRight.y - clearance) || CheckPointCollision(colliderPos, bottomLeft.y + clearance))
		{
			currentPosition.x = nextGridX + halfColliderSize.x;
			velocity.x = 0.f;
		}
		else
			currentPosition.x = nextPosition.x;
	}

	// Moving up
	if (isMovingUp)
	{
		float colliderPos = nextPosition.y + halfColliderSize.y;
		if (CheckPointCollision(topRight.x - clearance, colliderPos) || CheckPointCollision(bottomLeft.x + clearance, colliderPos))
		{
			currentPosition.y = nextGridY + 1.01f - halfColliderSize.y;
			velocity.y = 0.f;
		}
		else
			currentPosition.y = nextPosition.y;
	}
	// Moving down
	else
	{
		float colliderPos = nextPosition.y - halfColliderSize.y;
		if (CheckPointCollision(topRight.x - clearance, colliderPos) || CheckPointCollision(bottomLeft.x + clearance, colliderPos))
		{
			currentPosition.y = nextGridY + halfColliderSize.y;
			velocity.y = 0.f;
		}
		else
			currentPosition.y = nextPosition.y;
	}
}

int MapGrid::WorldToIndex(float x, float y)
{
	int gridX, gridY;
	WorldToGridCoords(AEVec2(x, y), gridX, gridY);

	if (gridX < 0 || gridX >= size.x || gridY < 0 || gridY >= size.y)
		return -1;

	return gridY * size.x + gridX;
}

int MapGrid::WorldToIndex(const AEVec2& worldPosition)
{
	int x, y;
	WorldToGridCoords(worldPosition, x, y);

	if (x < 0 || x >= size.x || y < 0 || y >= size.y)
		return -1;

	return y * size.x + x;
}

inline void MapGrid::WorldToGridCoords(const AEVec2& worldPosition, int& outX, int& outY)
{
	outX = (int)floorf(worldPosition.x);
	outY = (int)floorf(worldPosition.y);
}

inline void MapGrid::WorldToGridCoordsClamped(const AEVec2& worldPosition, int& outX, int& outY)
{
	outX = (int)AEClamp(floorf(worldPosition.x), 1, (float)size.x - 1);
	outY = (int)AEClamp(floorf(worldPosition.y), 1, (float)size.y - 1);
}
