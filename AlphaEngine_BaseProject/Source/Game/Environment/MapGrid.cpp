#include "MapGrid.h"
#include "../../Utils/MeshGenerator.h"
#include "../Camera.h"

MapGrid::MapGrid(int rows, int cols) : size(rows, cols), tiles(rows * cols)
{
	mesh = MeshGenerator::GetSquareMesh(1.f, 1.f / MapTile::typeCount, 1.f);
	tilemapTexture = AEGfxTextureLoad("Assets/Tmp/tmp-tilemap.png");
	
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
			tiles[y * size.x + x].type = MapTile::Type::GROUND;
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

			const MapTile& tile = GetTile(x, y);
			AEGfxTextureSet(tilemapTexture, (int)tile.type * (1.f / MapTile::typeCount), 1.f);
			AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
		}
	}
}

inline const MapTile& MapGrid::GetTile(int x, int y)
{
	return tiles[y * size.x + x];
}
