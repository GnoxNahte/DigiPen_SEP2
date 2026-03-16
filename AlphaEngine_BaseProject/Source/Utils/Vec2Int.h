#pragma once
#include <AEVec2.h>
#include <iostream>
#include <iomanip>

/**
 * @brief Similar to AEVec2 but this uses ints instead
 */
struct Vec2Int
{
	int x, y;
	
	Vec2Int();
	Vec2Int(const AEVec2& v);
	Vec2Int(int x, int y);

	AEVec2 GetAEVec2() const;

	static Vec2Int Max(const Vec2Int& lhs, const Vec2Int& rhs);

	// === With Vec2Int ===
	inline Vec2Int operator+(Vec2Int rhs) const
	{
		rhs.x += x;
		rhs.y += y;
		return rhs;
	}

	inline Vec2Int operator-(Vec2Int rhs) const
	{
		rhs.x = x - rhs.x;
		rhs.y = y - rhs.y;
		return rhs;
	}

	inline bool operator==(const Vec2Int& rhs) const
	{
		return x == rhs.x && y == rhs.y;
	}

	inline bool operator!=(const Vec2Int& rhs) const
	{
		return x != rhs.x || y != rhs.y;
	}

	// === With AEVec2 ===
	inline AEVec2 operator+(AEVec2 rhs) const
	{
		rhs.x += x;
		rhs.y += y;
		return rhs;
	}

	inline AEVec2 operator-(AEVec2 rhs) const
	{
		rhs.x = x - rhs.x;
		rhs.y = y - rhs.y;
		return rhs;
	}

	inline AEVec2 operator*(AEVec2 rhs) const
	{
		rhs.x *= x;
		rhs.y *= y;
		return rhs;
	}

	inline AEVec2 operator*(float rhs) const
	{
		return { x * rhs, y * rhs };
	}
};

inline AEVec2 operator+(AEVec2 lhs, const Vec2Int& rhs)
{
	return rhs + lhs;
}

inline AEVec2 operator-(AEVec2 lhs, const Vec2Int& rhs)
{
	lhs.x -= rhs.x;
	lhs.y -= rhs.y;
	return lhs;
}

inline std::ostream& operator<<(std::ostream& os, const Vec2Int& v)
{
	os << "(" << std::setw(2) << v.x << "," << v.y << ")";
	return os;
}
