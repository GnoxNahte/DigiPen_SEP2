#pragma once
#include "AEEngine.h"
#include <iostream>
#include <iomanip>

// Put this above so methods below can use the overloads
#pragma region AEVec2 Operator Overloads

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

inline AEVec2& operator*= (AEVec2& lhs, const AEVec2& rhs)
{
	lhs.x *= rhs.x;
	lhs.y *= rhs.y;
	return lhs;
}

inline AEVec2& operator*=(AEVec2& lhs, float rhs)
{
	lhs.x *= rhs;
	lhs.y *= rhs;
	return lhs;
}

inline AEVec2& operator/=(AEVec2& lhs, const AEVec2& rhs)
{
	lhs.x /= rhs.x;
	lhs.y /= rhs.y;
	return lhs;
}

inline AEVec2& operator/=(AEVec2& lhs, float rhs)
{
	lhs.x /= rhs;
	lhs.y /= rhs;
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

inline AEVec2 operator*(AEVec2 lhs, const AEVec2& rhs)
{
	lhs *= rhs;
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

inline AEVec2 operator/(AEVec2 lhs, const AEVec2& rhs)
{
	lhs /= rhs;
	return lhs;
}

inline AEVec2 operator/(float lhs, AEVec2 rhs)
{
	rhs.x = lhs / rhs.x;
	rhs.y = lhs / rhs.y;
	return rhs;
}

inline AEVec2 operator/(AEVec2 lhs, float rhs)
{
	lhs.x /= rhs;
	lhs.y /= rhs;
	return lhs;
}

inline bool operator==(const AEVec2& lhs, const AEVec2& rhs)
{
	return lhs.x == rhs.x && lhs.y == rhs.y;
}

inline bool operator!=(const AEVec2& lhs, const AEVec2& rhs)
{
	return lhs.x != rhs.x || lhs.y != rhs.y;
}

inline std::ostream& operator<<(std::ostream& os, const AEVec2& v)
{
	os << "(" << std::fixed << std::setprecision(3) << v.x << "," << v.y << ")";
	return os;
}

inline AEMtx33 operator*(AEMtx33 lhs, const AEMtx33& rhs)
{
	AEMtx33Concat(&lhs, &lhs, &rhs);
	return lhs;
}

#pragma endregion

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

	// Same as AEVec2SquareDistance but allows const 
	inline f32 SqrDist(const AEVec2& v)
	{
		return v.x * v.x + v.y * v.y;
	}

	// Same as AEVec2Distance but allows const 
	inline f32 Dist(const AEVec2& v)
	{
		return sqrtf(SqrDist(v));
	}

	// Return a normalised vector (Original vector unchanged)
	inline AEVec2 GetNormalise(const AEVec2& v)
	{
		return v / Dist(v);
	}

	// Normalise this vector
	inline AEVec2& Normalise(AEVec2& v)
	{
		return v /= Dist(v);
	}

	inline AEVec2 Abs(const AEVec2& v)
	{
		return { fabsf(v.x), fabsf(v.y) };
	}

	inline float Angle(const AEVec2& v)
	{
		return atan2f(v.y, v.x);
	}

	inline bool IsZero(const AEVec2& v)
	{
		return v.x == 0 && v.y == 0;
	}
}

template <typename T>
inline int Sign(T val)
{
	return (val > T(0)) - (val < T(0));
}

template <typename T>
inline int Sign_NoZero(T val)
{
	return (val >= T(0)) - (val < T(0));
}
