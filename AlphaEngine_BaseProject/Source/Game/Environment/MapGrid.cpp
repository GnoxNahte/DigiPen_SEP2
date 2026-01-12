#include <iostream>

#include "MapGrid.h"
#include "../../Utils/MeshGenerator.h"
#include "../Camera.h"

MapGrid::MapGrid(int rows, int cols) : size(rows, cols), tiles(rows * cols)
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
	for (int y = 0; y < size.y; y+=4)
	{
		for (int x = 0; x < size.y; x++)
		{
			tiles[y * size.x + x].type = x % 2 ? MapTile::Type::GROUND : MapTile::Type::NONE;
		}
	}
}

/* Calling it's own construct is tmp only. todo proper file read */
MapGrid::MapGrid(const char* ) : MapGrid(10, 10) 
{
}

void MapGrid::Render()
{
	AEMtx33 transform;

	for (int y = 0; y  < size.y; y ++)
	{
		for (int x = 0; x < size.y; x++)
		{
			// @todo - optimise this. init transform in the start and offset by a fixed amt (grid size * camera scale)? Then reset after for loop for x coords is done?
			// 
			// Local scale. For flipping sprite's facing direction
			AEMtx33Trans(&transform, (float)x, (float)y);
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
	int currX, currY, nextX, nextY;

	WorldToGridCoords(currentPosition, currX, currY);
	WorldToGridCoords(nextPosition, nextX, nextY);

	/*float left, right, top, bottom;

	if (nextPosition.x > currentPosition.x)
	{
		left = currX - colliderSize.x;
		right = nextX + colliderSize.x;
	}
	else
	{
		right = currX + colliderSize.x;
		left = nextX - colliderSize.x;
	}

	if (nextPosition.y > currentPosition.y)
	{
		top = currY - colliderSize.y;
		bottom = nextY + colliderSize.y;
	}
	else
	{
		bottom = currY + colliderSize.y;
		top = nextY - colliderSize.y;
	}*/

	// If collide
	if (CheckCollision(nextPosition) /*|| 
		CheckCollision(left, top) ||
		CheckCollision(right, top) ||
		CheckCollision(left, bottom) ||
		CheckCollision(right, bottom)*/)
	{
		std::cout << "HIT: ";
		if (nextPosition.x > currentPosition.x)
			currentPosition.x = nextX - colliderSize.x;
		else
			currentPosition.x = nextX + colliderSize.x;
		/*if (nextPosition.x > currentPosition.x)
			currentPosition.x = nextX - colliderSize.x;*/
	}
	// If doens't collide
	else
	{
		currentPosition = nextPosition;
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
	outX = (int)floorf(worldPosition.x + 0.5f);
	outY = (int)floorf(worldPosition.y + 0.5f);
}
