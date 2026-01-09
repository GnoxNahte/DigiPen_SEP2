#include "Camera.h"
#include "../Utils/Easing.h"

// Default camera scale
float Camera::scale = 1.f;

Camera::Camera(float xPos, float yPos, float _scale) :
	position(xPos, yPos), 
	offset(0, 0), 
	follow(nullptr),
	smoothTime(0.2f)
{
	Camera::scale = _scale;
}

void Camera::SetFollow(AEVec2* f, float xOffset, float yOffset, bool setPosToFollow)
{
	this->follow = f;
	this->offset.x = xOffset;
	this->offset.y = yOffset;

	if (setPosToFollow)
		position = *f;
}

void Camera::Update()
{
	float dt = (float)AEFrameRateControllerGetFrameTime();
	position = Easing::SmoothDamp(position, *follow, speed, smoothTime, dt);
	AEGfxSetCamPosition(position.x * Camera::scale, position.y * Camera::scale);
}
