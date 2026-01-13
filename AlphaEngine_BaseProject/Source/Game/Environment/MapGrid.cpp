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
			tiles[y * size.x + x].type = x % 4 ? MapTile::Type::GROUND : MapTile::Type::NONE;
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

bool MapGrid::CheckCollision(float x, float y)
{
	int index = WorldToIndex(x, y);
	if (index < 0)
		return false;

	return tiles[index].type != MapTile::Type::NONE;
}

bool MapGrid::CheckCollision(const AEVec2& worldPosition)
{
	return CheckCollision(worldPosition.x, worldPosition.y);
}

void MapGrid::HandleCollision(AEVec2& currentPosition, AEVec2& velocity, const AEVec2& nextPosition, const AEVec2& colliderSize)
{
	// This is a simple and quick collision handler. Should not use when (currentPosition - nextPosition).length > 1 tiles OR colliderSize > 1 tile
	// This also assumes currentPosition is empty
	// todo? proper line drawing? https://www.redblobgames.com/grids/line-drawing/
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

	if (AEInputCheckCurr(AEVK_V) && velocity.x > 0.f)
	{
		std::cout << "AAAAAAAAAAAAAAAAAAAAAA";
	}

	float clearance = 0.2f;

	// Moving right
	if (isMovingRight)
	{
		float colliderPos = nextPosition.x + halfColliderSize.x;
		if (CheckCollision(colliderPos, topRight.y - clearance) || CheckCollision(colliderPos, bottomLeft.y + clearance))
		{
			currentPosition.x = nextGridX + 1.01f - halfColliderSize.x;
			velocity.x = 0;
		}
		else
			currentPosition.x = nextPosition.x;
	}
	// Moving left
	else
	{
		float colliderPos = nextPosition.x - halfColliderSize.x;
		if (CheckCollision(colliderPos, topRight.y - clearance) || CheckCollision(colliderPos, bottomLeft.y + clearance))
		{
			currentPosition.x = nextGridX + halfColliderSize.x;
			velocity.x = 0;
		}
		else
			currentPosition.x = nextPosition.x;
	}

	// Moving up
	if (isMovingUp)
	{
		float colliderPos = nextPosition.y + halfColliderSize.y;
		if (CheckCollision(topRight.x - clearance, colliderPos) || CheckCollision(bottomLeft.x + clearance, colliderPos))
		{
			currentPosition.y = nextGridY + 1.01f - halfColliderSize.y;
			velocity.y = 0;
		}
		else
			currentPosition.y = nextPosition.y;
	}
	// Moving down
	else
	{
		float colliderPos = nextPosition.y - halfColliderSize.y;
		if (CheckCollision(topRight.x - clearance, colliderPos) || CheckCollision(bottomLeft.x + clearance, colliderPos))
		{
			currentPosition.y = nextGridY + halfColliderSize.y;
			velocity.y = 0;
		}
		else
			currentPosition.y = nextPosition.y;
	}


	// // If collide
	// if (CheckCollision(bottomLeft) ||
	// 	CheckCollision(topRight) ||
	// 	CheckCollision(bottomLeft) ||
	// 	CheckCollision(topRight))
	// {
	// 	std::cout << "Collide: ";
	// 	std::cout << currentPosition.x << ", " << currentPosition.y << " > ";
	// 	if (nextPosition.x > currentPosition.x)
	// 		currentPosition.x = nextGridX + 1.01f - halfColliderSize.x;
	// 	else
	// 		currentPosition.x = nextGridX + halfColliderSize.x;
	// 	std::cout << currentPosition.x << ", " << currentPosition.y << "\n";
	// 	/*if (nextPosition.x > currentPosition.x)
	// 		currentPosition.x = nextX - colliderSize.x;*/
	// }
	// // If doens't collide
	// else
	// {
	// 	std::cout << "No collide: ";
	// 	std::cout << currentPosition.x << ", " << currentPosition.y << " > ";
	// 	currentPosition = nextPosition;
	// 	std::cout << currentPosition.x << ", " << currentPosition.y << "\n";
	// }
}

int MapGrid::WorldToIndex(float x, float y)
{
	int gridX, gridY;
	WorldToGridCoords(x, y, gridX, gridY);

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

inline void MapGrid::WorldToGridCoords(float worldX, float worldY, int& outX, int& outY)
{
	outX = (int)floorf(worldX);
	outY = (int)floorf(worldY);
}