#pragma once
#include "AEEngine.h"

class Camera
{
public:
	static float scale;

	AEVec2 position;

	AEVec2 offset;
	AEVec2* follow;
	AEVec2 deadzoneRange; // @todo

	AEVec2 speed;
	float smoothTime;

	Camera(float xPos, float yPos, float _scale);

	void SetFollow(AEVec2* follow, float xOffset, float yOffset, bool setPosToFollow);

	void Update();
};
