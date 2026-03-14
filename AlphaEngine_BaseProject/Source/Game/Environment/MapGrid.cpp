#include <iostream>

#include "MapGrid.h"
#include "../../Utils/MeshGenerator.h"
#include "../Camera.h"
#include "../../Utils/AEExtras.h"

namespace
{
	static constexpr const char* SURFACE_PATH = "Assets/Tmp/tile_surface.png";
	static constexpr const char* BODY_PATH = "Assets/Tmp/tile_middle.png";
	static constexpr const char* BOTTOM_PATH = "Assets/Tmp/tile_bottom.png";
	static constexpr const char* PLATFORM_PATH = "Assets/Tmp/platform.png";

	// platform behaves as a flat 5-cell-wide platform
	static constexpr float PLATFORM_WIDTH_TILES = 1.0f;
	static constexpr float PLATFORM_HEIGHT_TILES = 1.0f;
	static constexpr int   PLATFORM_COLLISION_WIDTH = 1;
}

MapGrid::MapGrid(int cols, int rows)
	: size(cols, rows),
	tiles(cols* rows),
	tileCount(cols* rows)
{
	// full texture on a 1x1 quad
	tileMesh = MeshGenerator::GetSquareMesh(1.f, 1.f, 1.f);

	surfaceTexture = AEGfxTextureLoad(SURFACE_PATH);
	bodyTexture = AEGfxTextureLoad(BODY_PATH);
	bottomTexture = AEGfxTextureLoad(BOTTOM_PATH);
	platformTexture = AEGfxTextureLoad(PLATFORM_PATH);

	for (auto& t : tiles)
		t.type = MapTile::Type::NONE;
}

MapGrid::MapGrid(const char*) : MapGrid(10, 10)
{
}

MapGrid::~MapGrid()
{
	if (tileMesh)
		AEGfxMeshFree(tileMesh);

	if (surfaceTexture)
		AEGfxTextureUnload(surfaceTexture);

	if (bodyTexture)
		AEGfxTextureUnload(bodyTexture);

	if (bottomTexture)
		AEGfxTextureUnload(bottomTexture);

	if (platformTexture)
		AEGfxTextureUnload(platformTexture);
}

bool MapGrid::IsSolidAtGridCell(int x, int y) const
{
	if (x < 0 || x >= size.x || y < 0 || y >= size.y)
		return false;

	const MapTile::Type here = tiles[y * size.x + x].type;

	// normal solid tiles occupy exactly one cell
	if (here == MapTile::Type::GROUND_SURFACE ||
		here == MapTile::Type::GROUND_BODY ||
		here == MapTile::Type::GROUND_BOTTOM)
		return true;

	// platform anchor occupies its whole 5-cell span on the same row
	if (here == MapTile::Type::PLATFORM)
		return true;

	// check if this cell is covered by a platform anchor to its left
	for (int anchorX = x - (PLATFORM_COLLISION_WIDTH - 1); anchorX <= x; ++anchorX)
	{
		if (anchorX < 0 || anchorX >= size.x)
			continue;

		if (tiles[y * size.x + anchorX].type == MapTile::Type::PLATFORM)
		{
			if (x >= anchorX && x < anchorX + PLATFORM_COLLISION_WIDTH)
				return true;
		}
	}

	return false;
}

void MapGrid::Render()
{
	AEMtx33 transform;

	AEVec2 bottomLeft, topRight;
	AEExtras::ScreenToWorldPosition(AEVec2(0, (f32)AEGfxGetWindowHeight()), bottomLeft);
	AEExtras::ScreenToWorldPosition(AEVec2((f32)AEGfxGetWindowWidth(), 0), topRight);

	topRight.x += 1.f;
	topRight.y += 1.f;

	int minX, minY, maxX, maxY;
	WorldToGridCoordsClamped(bottomLeft, minX, minY);
	WorldToGridCoordsClamped(topRight, maxX, maxY);

	for (int y = minY; y <= maxY; ++y)
	{
		for (int x = minX; x <= maxX; ++x)
		{
			const MapTile* tile = GetTile(x, y);
			if (tile == nullptr || tile->type == MapTile::Type::NONE)
				continue;

			AEGfxTexture* tex = nullptr;
			float scaleX = Camera::scale;
			float scaleY = Camera::scale;
			float drawX = (float)x + 0.5f;
			float drawY = (float)y + 0.5f;

			switch (tile->type)
			{
			case MapTile::Type::GROUND_SURFACE:
				tex = surfaceTexture;
				break;

			case MapTile::Type::GROUND_BODY:
				tex = bodyTexture;
				break;

			case MapTile::Type::GROUND_BOTTOM:
				tex = bottomTexture;
				break;

			case MapTile::Type::PLATFORM:
				tex = platformTexture;

				// one platform tile is the LEFT anchor of a 5x1 platform
				scaleX = Camera::scale * PLATFORM_WIDTH_TILES;
				scaleY = Camera::scale * PLATFORM_HEIGHT_TILES;

				drawX = (float)x + (PLATFORM_WIDTH_TILES * 0.5f);
				drawY = (float)y + 0.5f;
				break;

			default:
				break;
			}

			if (!tex)
				continue;

			AEMtx33Trans(&transform, drawX, drawY);
			AEMtx33ScaleApply(&transform, &transform, scaleX, scaleY);
			AEGfxSetTransform(transform.m);

			AEGfxTextureSet(tex, 0.0f, 0.0f);
			AEGfxMeshDraw(tileMesh, AE_GFX_MDM_TRIANGLES);
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

void MapGrid::SetTile(int x, int y, MapTile::Type type)
{
	if (x < 0 || x >= size.x || y < 0 || y >= size.y)
		return;

	tiles[y * size.x + x].type = type;
}

bool MapGrid::CheckPointCollision(float x, float y)
{
	int gridX, gridY;
	WorldToGridCoords(AEVec2(x, y), gridX, gridY);
	return IsSolidAtGridCell(gridX, gridY);
}

bool MapGrid::CheckPointCollision(const AEVec2& worldPosition)
{
	return CheckPointCollision(worldPosition.x, worldPosition.y);
}

bool MapGrid::CheckBoxCollision(const AEVec2& boxPosition, const AEVec2& boxSize)
{
	AEVec2 halfBoxSize(boxSize.x * 0.5f, boxSize.y * 0.5f);

	return CheckPointCollision(boxPosition.x - halfBoxSize.x, boxPosition.y - halfBoxSize.y) ||
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
	AEVec2 halfColliderSize(colliderSize.x * 0.5f, colliderSize.y * 0.5f);

	AEVec2 bottomLeft, topRight;
	bottomLeft.x = min(currentPosition.x, nextPosition.x) - halfColliderSize.x;
	bottomLeft.y = min(currentPosition.y, nextPosition.y) - halfColliderSize.y;
	topRight.x = max(currentPosition.x, nextPosition.x) + halfColliderSize.x;
	topRight.y = max(currentPosition.y, nextPosition.y) + halfColliderSize.y;

	int nextGridX, nextGridY;
	WorldToGridCoords(nextPosition, nextGridX, nextGridY);

	bool isMovingRight = nextPosition.x > currentPosition.x;
	bool isMovingUp = nextPosition.y > currentPosition.y;

	float clearance = 0.2f;

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
	outX = (int)AEClamp(floorf(worldPosition.x), 0, (float)size.x - 1);
	outY = (int)AEClamp(floorf(worldPosition.y), 0, (float)size.y - 1);
}