#pragma once
#include "AEEngine.h"
#include "Player/Player.h"

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

	void SetFollow(AEVec2* follow, float xOffset, float yOffset);

	void Update();
	// No Render() as nothing to render
};
