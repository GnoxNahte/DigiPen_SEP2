#pragma once
#include <vector>
#include <array>
#include "AEEngine.h"

#include "MapTile.h"
#include "../../Utils/Vec2Int.h"

class MapGrid
{
public:
	MapGrid(int rows, int cols);
	MapGrid(const char* file);

	void Render();

	inline const MapTile& GetTile(int x, int y);
private:
	std::vector<MapTile> tiles;
	Vec2Int size;

	AEGfxVertexList* mesh;
	AEGfxTexture* tilemapTexture;


	//std::array<AEVec2, MapTile::MAP_TILE_TYPE_COUNT> uvCoords;
};

