#pragma once
#include "AEEngine.h"


class Camera
{
public:
	static float scale;
	static AEVec2 position;

	AEVec2 offset;
	const AEVec2* follow;

	AEVec2 minBounds;
	AEVec2 maxBounds;
	AEVec2 deadzoneRange; // @todo

	// === Room-based camera data (Celeste-style) ===
	// halfView: half the screen size in world units (x = half-width, y = half-height)
	AEVec2 halfView;
	// roomSize: full screen size in world units
	AEVec2 roomSize;
	// roomTarget: camera center for the current room (camera eases toward this)
	AEVec2 roomTarget;

	AEVec2 velocity;
	float smoothTime;

	Camera(const AEVec2& minBounds, const AEVec2& maxBounds, float _scale);

	void SetFollow(const AEVec2* follow, float xOffset, float yOffset, bool setPosToFollow);

	void Update();
};
