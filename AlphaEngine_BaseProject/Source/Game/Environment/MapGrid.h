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

	inline const MapTile* GetTile(int x, int y);

	/**
	 * @brief	Check if the tile is occupied by a tile
	 * @param x Grid x coordinate
	 * @param y Grid y coordinate
	 * @return	Returns if it's occupied by a tile
	 */
	bool CheckCollision(int x, int y);

	/**
	 * @brief				Check if that world position is occupied by a tile
	 * @param worldPosition World position to check
	 * @return				Returns there's a tile in that position
	 */
	bool CheckCollision(const AEVec2& worldPosition);

	/**
	 * @brief					Handles collision. Tries to move to nextPosition. If cannot, will try to move to the space closest to it
	 * @param currentPosition	Reference to current position. Changes this
	 * @param nextPosition		Next desired position
	 */
	void HandleCollision(AEVec2& currentPosition, const AEVec2& nextPosition, const AEVec2& size);
private:
	std::vector<MapTile> tiles;
	Vec2Int size;
	int tileCount; // size.x * size.y

	AEGfxVertexList* mesh;
	AEGfxTexture* tilemapTexture;
	//std::array<AEVec2, MapTile::MAP_TILE_TYPE_COUNT> uvCoords;

	/**
	 * @brief				Converts from world position to index for tiles array
	 * @param worldPosition World position
	 * @return				Index in tiles array
	 */
	int WorldToIndex(const AEVec2& worldPosition);

	/**
	 * @brief				Converts from world position to index for tiles array 
	 * @param worldPosition World position
	 * @param outX			Outputs grid x coordinates
	 * @param outY			Outputs grid y coordinates
	 */
	inline void WorldToGridCoords(const AEVec2& worldPosition, int& outX, int& outY);
};

