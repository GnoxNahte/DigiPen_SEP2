#pragma once
#include <vector>
#include "AEEngine.h"

#include "MapTile.h"
#include "../../Utils/Vec2Int.h"
#include "../../Utils/Box.h"
#include "../Camera.h"

class MapGrid
{
public:
	MapGrid(int cols, int rows);
	MapGrid(const char* file);
	~MapGrid();

	void Render();

	inline const MapTile* GetTile(int x, int y);
	void SetTile(int x, int y, MapTile::Type type);

	bool CheckPointCollision(float x, float y);
	bool CheckPointCollision(const AEVec2& worldPosition);

	bool CheckBoxCollision(const AEVec2& boxPosition, const AEVec2& boxSize);
	bool CheckBoxCollision(const Box& box);

	void HandleBoxCollision(AEVec2& currentPosition, AEVec2& velocity, const AEVec2& nextPosition, const AEVec2& size);

private:
	bool IsSolidAtGridCell(int x, int y) const;

private:
	std::vector<MapTile> tiles;
	Vec2Int size;
	int tileCount;

	AEGfxVertexList* tileMesh = nullptr;

	AEGfxTexture* surfaceTexture = nullptr;
	AEGfxTexture* bodyTexture = nullptr;    // tile_middle.png
	AEGfxTexture* bottomTexture = nullptr;  // tile_bottom.png
	AEGfxTexture* platformTexture = nullptr;


	int WorldToIndex(const AEVec2& worldPosition);
	int WorldToIndex(float x, float y);

	inline void WorldToGridCoords(const AEVec2& worldPosition, int& outX, int& outY);
	inline void WorldToGridCoordsClamped(const AEVec2& worldPosition, int& outX, int& outY);
};