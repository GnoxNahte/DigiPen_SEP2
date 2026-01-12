#include <iostream>

#include "MapGrid.h"
#include "../../Utils/MeshGenerator.h"
#include "../Camera.h"

MapGrid::MapGrid(int rows, int cols) : size(rows, cols), tiles(rows* cols)
{
	mesh = MeshGenerator::GetSquareMesh(1.f, 1.f / MapTile::typeCount, 1.f);
	tilemapTexture = AEGfxTextureLoad("Assets/Tmp/tmp-tilemap.png");

	tileCount = size.x * size.y;

	// for (int i = 0; i < uvCoordsSize; ++i)
	// {
	// 	uvCoords[i].x = 1.f / uvCoordsSize;
	// 	uvCoords[i].y = 1.f;
	// }

	// Just for testing
	for (int y = 0; y < size.y; y += 4)
	{
		for (int x = 0; x < size.y; x++)
		{
			tiles[y * size.x + x].type = x % 3 ? MapTile::Type::NONE : MapTile::Type::GROUND;
		}
	}
}

/* Calling it's own construct is tmp only. todo proper file read */
MapGrid::MapGrid(const char*) : MapGrid(10, 10)
{
}

void MapGrid::Render()
{
	AEMtx33 transform;

	for (int y = 0; y < size.y; y++)
	{
		for (int x = 0; x < size.y; x++)
		{
			// @todo - optimise this. init transform in the start and offset by a fixed amt (grid size * camera scale)? Then reset after for loop for x coords is done?
			// 
			// Local scale. For flipping sprite's facing direction
			AEMtx33Trans(&transform, (float)(x + 0.5f), (float)(y + 0.5f));
			// Camera scale. Scales translation too.
			AEMtx33ScaleApply(&transform, &transform, Camera::scale, Camera::scale);
			AEGfxSetTransform(transform.m);

			const MapTile& tile = *GetTile(x, y);
			AEGfxTextureSet(tilemapTexture, (int)tile.type * (1.f / MapTile::typeCount), 1.f);
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

bool MapGrid::CheckCollision(int x, int y)
{
	return (*GetTile(x, y)).type != MapTile::Type::NONE;
}

bool MapGrid::CheckCollision(const AEVec2& worldPosition)
{
	int index = WorldToIndex(worldPosition);
	if (index < 0)
		return false;

	return tiles[index].type != MapTile::Type::NONE;
}

void MapGrid::HandleCollision(AEVec2& currentPosition, const AEVec2& nextPosition, const AEVec2& colliderSize)
{
	// This is a simple and quick collision handler. Should not use when (currentPosition - nextPosition).length > 1 tiles OR colliderSize > 1 tile
	// This also assumes currentPosition is empty
	// todo? proper line drawing? https://www.redblobgames.com/grids/line-drawing/

	AEVec2 halfColliderSize(colliderSize.x * 0.5f, colliderSize.y * 0.5f);

	if (currentPosition.x < 0 || currentPosition.x > size.x ||
		currentPosition.y < 0 || currentPosition.x > size.y ||
		nextPosition.x < 0 || nextPosition.x > size.x ||
		nextPosition.y < 0 || nextPosition.x > size.y)
	{
		// todo handle properly
		currentPosition = nextPosition;
		return;
	}

	// abit messy
	AEVec2 topLeft, bottomRight;
	topLeft.x = min(currentPosition.x, nextPosition.x) - halfColliderSize.x,
	topLeft.y = min(currentPosition.y, nextPosition.y) - halfColliderSize.y,
	bottomRight.x = max(currentPosition.x, nextPosition.x) + halfColliderSize.x,
	bottomRight.y = max(currentPosition.y, nextPosition.y) + halfColliderSize.y;

	AEVec2	topRight(bottomRight.x, topLeft.y),
		bottomLeft(topLeft.x, bottomRight.y);

	// If collide
	if (CheckCollision(topLeft) ||
		CheckCollision(topRight) ||
		CheckCollision(bottomLeft) ||
		CheckCollision(bottomRight))
	{
		int nextX, nextY;

		WorldToGridCoords(nextPosition, nextX, nextY);
		std::cout << "Collide: ";
		std::cout << currentPosition.x << ", " << currentPosition.y << " > ";
		if (nextPosition.x > currentPosition.x)
			currentPosition.x = nextX + 1.01f - halfColliderSize.x;
		else
			currentPosition.x = nextX + halfColliderSize.x;
		std::cout << currentPosition.x << ", " << currentPosition.y << "\n";
		/*if (nextPosition.x > currentPosition.x)
			currentPosition.x = nextX - colliderSize.x;*/
	}
	// If doens't collide
	else
	{
		std::cout << "No collide: ";
		std::cout << currentPosition.x << ", " << currentPosition.y << " > ";
		currentPosition = nextPosition;
		std::cout << currentPosition.x << ", " << currentPosition.y << "\n";
	}
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