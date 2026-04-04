/*!
@file	Vec2Int.cpp
@author	Ethan Ong
@brief	Defines (and defines some inline functions) Vec2Int,
		Simliar to AEVec2 but the base type is int instead of float

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/

#include "Vec2Int.h"
#include <algorithm>

Vec2Int::Vec2Int() : x(0), y(0) { }

Vec2Int::Vec2Int(const AEVec2& v) : 
	x(static_cast<int>(v.x)),
	y(static_cast<int>(v.y))
{
}

Vec2Int::Vec2Int(int x, int y) : x(x), y(y) { }

AEVec2 Vec2Int::GetAEVec2() const
{
	return AEVec2(
		static_cast<float>(x), 
		static_cast<float>(y)
	);
}

Vec2Int Vec2Int::Max(const Vec2Int& lhs, const Vec2Int& rhs)
{
	return {
		std::max(lhs.x, rhs.x),
		std::max(lhs.y, rhs.y)
	};
}
