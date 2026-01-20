#pragma once
#include "AEEngine.h"

class AEExtras
{
public:
	/**
	 * @brief				 Gets the cursor's world position
	 * @param outPosition	 Outputs position here
	 * @param cameraPosition Current camera position
	 */
	static void GetCursorWorldPosition(AEVec2& outPosition, const AEVec2& cameraPosition);

	/**
	 * @brief					Converts a position from screen coordinates to world coordinates.
	 * @param screenPosition	Input screen position
	 * @param cameraPosition	Current camera position
	 * @param outWorldPosition	Output world coordinates
	 */
	static void ScreenToWorldPosition(const AEVec2& screenPosition, const AEVec2& cameraPosition, AEVec2& outWorldPosition);

	/**
	 * @brief		Returns a float between the range
	 */
	static float RandomRange(const AEVec2& range);
	//static float Remap(float value, const AEVec2& inRange, const AEVec2& outRange);
};

