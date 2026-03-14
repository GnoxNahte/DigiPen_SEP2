#include <iostream>
#include <algorithm>

#include "MapGrid.h"
#include "../../Utils/MeshGenerator.h"
#include "../Camera.h"
#include "../../Utils/AEExtras.h"
#include "../../Utils/QuickGraphics.h"
#include "../../Editor/Editor.h"

#undef min
#undef max

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

float MapGrid::Raycast(const AEVec2& start, const AEVec2& end)
{
	if (start == end)
		return 0.f;

	// Algorithm: Amanatides and Woo
	// Reference: https://m4xc.dev/articles/amanatides-and-woo/
	AEVec2 ray{ end - start };
	float rayDist = AEExtras::Dist(ray);

	Vec2Int step{ Sign(ray.x), Sign(ray.y) };

	Vec2Int currCell{ start };

	if (IsSolidAtGridCell(currCell.x, currCell.y))
		return 0.f;

	// tMax: Percentage of each axis from start to end
	// 1. currCell - start = amt moved from start to curr cell. (Floating point of start cell for now, since cell size == 1)
	// 2. Vec2Int::Max(step, {0,0}) = If positive (in their own axis), start from the next cell
	// 3. <result> / ray = Percentage of the ray 
	//
	// delta: Amt to move each step
	AEVec2	tMax{ (currCell - start + Vec2Int::Max(step, {0,0})) / ray },
			delta{ AEExtras::Abs(1.f / ray) };

	// When divide by 0
	if (ray.x == 0.f)
	{
		tMax.x = std::numeric_limits<f32>::max();
		delta.x = std::numeric_limits<f32>::max();
	}
	if (ray.y == 0.f)
	{
		tMax.y = std::numeric_limits<f32>::max();
		delta.y = std::numeric_limits<f32>::max();
	}

	bool isAxisX = true;
	while (tMax.x <= 1.f + delta.x && tMax.y <= 1.f + delta.y)
	{
		if (IsSolidAtGridCell(currCell.x, currCell.y))
		{
			if (Editor::GetShowColliders())
			{
				QuickGraphics::DrawRay(start, start + (isAxisX ? (tMax.x - delta.x) : (tMax.y - delta.y)) * ray, 0.1f, 0xAAFF8800);
				QuickGraphics::DrawRect(currCell.GetAEVec2() + AEVec2{0.5f, 0.5f}, { 1.f, 1.f }, 0xFFFF0000, AE_GFX_MDM_LINES_STRIP);
			}

			if (isAxisX)
				return (tMax.x - delta.x) * rayDist;
			else
				return (tMax.y - delta.y) * rayDist;
		}

		if (Editor::GetShowColliders())
			QuickGraphics::DrawRect(currCell.GetAEVec2() + AEVec2{0.5f, 0.5f}, { 1.f, 1.f }, 0xFF00FF00, AE_GFX_MDM_LINES_STRIP);

		// Step on the axis where tMax is the smallest
		if (tMax.x < tMax.y)
		{
			tMax.x += delta.x;
			currCell.x += step.x;
			isAxisX = true;
		}
		else
		{
			tMax.y += delta.y;
			currCell.y += step.y;
			isAxisX = false;
		}
	}

	if (Editor::GetShowColliders())
		QuickGraphics::DrawRay(start, end, 0.1f, 0xAA88FF88);

	return rayDist;
}

// Not the most efficient algorithm but not sure how else to do dynamic collision with a grid...
// Also doesn't handle properly if colliderSize > 2
// Rough explanation
// - Raycasts from 4 vertices of the box
// - Get min dist from the raycasts
// - Make it slide
void MapGrid::HandleBoxCollision(AEVec2& currPosition, AEVec2& , const AEVec2& nextPosition, const AEVec2& colliderSizeTmp, bool ifSlide)
{
	if (currPosition == nextPosition)
		return;

	//float epsilon = EPSILON;
	constexpr float epsilon = 0.01f;

	AEVec2 colliderSize = colliderSizeTmp - AEVec2{ epsilon, epsilon };
	AEVec2 halfColliderSize = colliderSize * 0.5f;

	AEVec2 ray = nextPosition - currPosition;
	float rayDist = AEExtras::Dist(ray);
	AEVec2 rayDir = ray / rayDist;

	Vec2Int raySign{ Sign_NoZero(rayDir.x), Sign_NoZero(rayDir.y) };

	float dist = rayDist;
	AEVec2 corner{ currPosition + raySign * halfColliderSize };
	dist = std::min(dist, Raycast(corner, corner + ray));

	corner.x -= colliderSize.x * raySign.x;
	dist = std::min(dist, Raycast(corner, corner + ray));
	
	corner.x += colliderSize.x * raySign.x; // Revert previous offset
	corner.y -= colliderSize.y * raySign.y;
	dist = std::min(dist, Raycast(corner, corner + ray));

	// EPSILON to prevent clipping into the tile
	currPosition += rayDir * dist - Vec2Int{ Sign(rayDir.x), Sign(rayDir.y) } * epsilon;

	// ===== Sliding =====
	if (!ifSlide || dist == rayDist)
		return;
	AEVec2 remainingRay = rayDir * (rayDist - dist);

	// Check raycast for x (on the leading side and in that direction)
	if (std::abs(remainingRay.x) > epsilon)
	{
		AEVec2 point = currPosition + raySign * halfColliderSize;
		float x1 = Raycast(point, { point.x + remainingRay.x, point.y });

		point.y -= colliderSize.y * raySign.y;
		float x2 = Raycast(point, { point.x + remainingRay.x, point.y });

		remainingRay.x = std::min(x1, x2) * raySign.x;
	}

	// Check raycast for y (on the leading side and in that direction)
	if (std::abs(remainingRay.y) > epsilon)
	{
		AEVec2 point = currPosition + raySign * halfColliderSize;
		float y1 = Raycast(point, { point.x, point.y + remainingRay.y });

		point.y -= colliderSize.y * raySign.y;
		float y2 = Raycast(point, { point.x, point.y + remainingRay.y });

		remainingRay.y = std::min(y1, y2) * raySign.y;
	}

	currPosition += remainingRay;
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