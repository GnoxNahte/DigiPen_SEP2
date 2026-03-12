#pragma once
#include "AEEngine.h"
#include <iostream>

namespace AEExtras
{
	/**
	 * @brief				 Gets the cursor's world position
	 * @param outPosition	 Outputs position here
	 */
	void GetCursorWorldPosition(AEVec2& outPosition);

	/**
	 * @brief					Converts a position from screen coordinates to world coordinates.
	 * @param screenPosition	Input screen position
	 * @param outWorldPosition	Output world coordinates
	 */
	void ScreenToWorldPosition(const AEVec2& screenPosition, AEVec2& outWorldPosition);

	/**
	 * @brief					Converts a position from world coordinates to screen coordinates.
	 * @param worldPosition		Input world position
	 * @param outScreenPosition	Output screen coordinates
	 */
	void WorldToScreenPosition(const AEVec2& worldPosition, AEVec2& outScreenPosition);

	/**
	 * @brief						Converts a position from world coordinates to viewport coordinates.
	 * @param worldPosition			Input screen position
	 * @param outViewportPosition	Output world coordinates
	 */
	void WorldToViewportPosition(const AEVec2& worldPosition, AEVec2& outViewportPosition);

	/**
	 * @brief		Returns a float between the range
	 */
	float RandomRange(const AEVec2& range);

	float Remap(float value, const AEVec2& inRange, const AEVec2& outRange);
	float RemapClamp(float value, const AEVec2& inRange, const AEVec2& outRange);

	// Same as AEVec2Distance but allows const 
	inline f32 Dist(const AEVec2& lhs, const AEVec2& rhs)
	{
		return sqrtf(SqrDist(lhs, rhs));
	}

	// Same as AEVec2SquareDistance but allows const 
	inline f32 SqrDist(const AEVec2& lhs, const AEVec2& rhs)
	{
		return (rhs.x - lhs.x) * (rhs.x - lhs.x) + (rhs.y - lhs.y) * (rhs.y - lhs.y)
	}
}

template <typename T>
inline int sign(T val)
{
	return (val > T(0)) - (val < T(0));
}

// === AEVec2 operator overloads ===
// NOTE: the operators that return a temp value (+, -, *) take in lvalue instead of rvalue
//		 For return value optimisation/copy elision - prevents copies. But not sure since haven't learn... Maybe use rvalue reference instead?
// Src:
// - https://stackoverflow.com/questions/21605579/how-true-is-want-speed-pass-by-value
// - https://web.archive.org/web/20140205194657/http://cpp-next.com/archive/2009/08/want-speed-pass-by-value/

inline AEVec2& operator+=(AEVec2& lhs, const AEVec2& rhs)
{
	lhs.x += rhs.x;
	lhs.y += rhs.y;
	return lhs;
}

inline AEVec2& operator-=(AEVec2& lhs, const AEVec2& rhs)
{
	lhs.x -= rhs.x;
	lhs.y -= rhs.y;
	return lhs;
}

inline AEVec2& operator*=(AEVec2& lhs, float rhs)
{
	lhs.x *= rhs;
	lhs.y *= rhs;
	return lhs;
}

inline AEVec2 operator+(AEVec2 lhs, const AEVec2& rhs)
{
	lhs += rhs;
	return lhs;
}

inline AEVec2 operator-(AEVec2 lhs, const AEVec2& rhs)
{
	lhs -= rhs;
	return lhs;
}

inline AEVec2 operator*(AEVec2 lhs, float rhs)
{
	lhs *= rhs;
	return lhs;
}

inline AEVec2 operator*(float lhs, AEVec2 rhs)
{
	rhs *= lhs;
	return rhs;
}

inline std::ostream& operator<<(std::ostream& os, const AEVec2& v)
{
	os << "(" << v.x << "," << v.y << ")";
	return os;
}

