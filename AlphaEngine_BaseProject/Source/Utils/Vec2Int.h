#pragma once
#include <AEVec2.h>
/**
 * @brief Similar to AEVec2 but this uses ints instead
 */
struct Vec2Int
{
	int x, y;
	Vec2Int();
	Vec2Int(const AEVec2& v);
	Vec2Int(int x, int y);
};

