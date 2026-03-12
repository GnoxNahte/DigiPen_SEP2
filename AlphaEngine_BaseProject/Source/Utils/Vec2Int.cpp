#include "Vec2Int.h"

Vec2Int::Vec2Int() : x(0), y(0) { }

Vec2Int::Vec2Int(const AEVec2& v) : 
	x(static_cast<int>(v.x)),
	y(static_cast<int>(v.y))
{
}

Vec2Int::Vec2Int(int x, int y) : x(x), y(y) { }
