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
	MapGrid(int rows, int cols);
	MapGrid(const char* file);

	~MapGrid();

	void Render(const Camera& camera);

	inline const MapTile* GetTile(int x, int y);

	//for level editor
	void SetTile(int x, int y, MapTile::Type type);

	/**
	 * @brief	Check if that world position is occupied by a tile
	 * @param x World x position
	 * @param y World y position
	 * @return	Returns there's a tile in that position
	 */
	bool CheckPointCollision(float x, float y);

	/**
	 * @brief				Check if that world position is occupied by a tile
	 * @param worldPosition World position to check
	 * @return				Returns there's a tile in that position
	 */
	bool CheckPointCollision(const AEVec2& worldPosition);

	bool CheckBoxCollision(const AEVec2& boxPosition, const AEVec2& boxSize);
	bool CheckBoxCollision(const Box& box);

	/**
	 * @brief					Handles collision. Tries to move to nextPosition. If cannot, will try to move to the space closest to it
	 * @param velocity			Reference to velocity of current object
	 * @param currentPosition	Reference to current position
	 * @param nextPosition		Next desired position
	 * @param size				Collider/Box size
	 */
	void HandleBoxCollision(AEVec2& currentPosition, AEVec2& velocity, const AEVec2& nextPosition, const AEVec2& size);
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

	int WorldToIndex(float x, float y);

	/**
	 * @brief				Converts from world position to index for tiles array. Unclamped out grid
	 * @param worldPosition World position
	 * @param outX			Outputs grid x coordinates
	 * @param outY			Outputs grid y coordinates
	 */
	inline void WorldToGridCoords(const AEVec2& worldPosition, int& outX, int& outY);

	/**
	 * @brief				Converts from world position to index for tiles array, clamped grid to [0, size]
	 * @param worldPosition World position
	 * @param outX			Outputs grid x coordinates
	 * @param outY			Outputs grid y coordinates
	 */
	inline void WorldToGridCoordsClamped(const AEVec2& worldPosition, int& outX, int& outY);
};

