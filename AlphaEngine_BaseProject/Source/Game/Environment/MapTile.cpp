/*!
@file	MapTile.cpp
@author	Ethan Ong, Santhosh
@brief	Defines a MapTile class for MapGrid

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/

#include "MapTile.h"

MapTile::MapTile() : type(Type::NONE)
{
}

MapTile::MapTile(Type type) : type(type)
{

}
