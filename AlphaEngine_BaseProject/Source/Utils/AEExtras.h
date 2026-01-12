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
};

