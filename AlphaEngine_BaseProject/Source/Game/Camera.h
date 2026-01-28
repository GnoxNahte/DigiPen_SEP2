#pragma once
#include "AEEngine.h"

class Camera
{
public:
	static float scale;

	AEVec2 position;

	AEVec2 offset;
	const AEVec2* follow;

	AEVec2 minBounds;
	AEVec2 maxBounds;
	AEVec2 deadzoneRange; // @todo

	AEVec2 velocity;
	float smoothTime;

	Camera(const AEVec2& minBounds, const AEVec2& maxBounds, float _scale);

	void SetFollow(const AEVec2* follow, float xOffset, float yOffset, bool setPosToFollow);

	void Update();
};
