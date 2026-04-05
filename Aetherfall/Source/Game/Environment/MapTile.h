/*!
@file	MapTile.h
@author	Ethan Ong, Santhosh
@brief	Declares a MapTile class for MapGrid

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/

#pragma once

struct MapTile
{
public:
	enum Type
	{
		NONE = 0,
		GROUND_SURFACE,
		GROUND_BODY,   
		GROUND_BOTTOM, 
		PLATFORM,
		

		MAP_TILE_TYPE_COUNT,
	};

	static const int typeCount = (int)MAP_TILE_TYPE_COUNT;

	Type type;

	MapTile();
	MapTile(Type type);
};