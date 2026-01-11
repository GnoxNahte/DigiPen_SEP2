#pragma once

struct MapTile
{
public:
	enum Type
	{
		NONE= 0,
		GROUND,

		MAP_TILE_TYPE_COUNT,
	};

	static const int typeCount = (int)MAP_TILE_TYPE_COUNT;

	Type type;

	MapTile();
	MapTile(Type type);
};

