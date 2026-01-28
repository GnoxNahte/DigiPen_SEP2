#include "Camera.h"
#include "../Utils/AEExtras.h"
#include "../Utils/Easing.h"

// Default camera scale
float Camera::scale = 1.f;

Camera::Camera(const AEVec2& minBounds, const AEVec2& maxBounds, float _scale) :
	position(0.f, 0.f), 
	offset(0, 0), 
	follow(nullptr),
	smoothTime(0.2f),
	deadzoneRange(0, 0),
	velocity(0)
{
	Camera::scale = _scale;

	// Top left and middle of the screen in world space
	AEVec2 topLeft, middle;
	AEExtras::ScreenToWorldPosition({ 0.f, 0.f }, position, topLeft);
	AEExtras::ScreenToWorldPosition(
		{ AEGfxGetWindowWidth() * 0.5f, AEGfxGetWindowHeight() * 0.5f }, 
		position, 
		middle
	);

	// Distance between the middle and topLeft
	AEVec2 distAmt;
	AEVec2Sub(&distAmt, &middle, &topLeft);

	this->minBounds.x = minBounds.x + distAmt.x;
	this->minBounds.y = minBounds.y + distAmt.y;

	this->maxBounds.x = maxBounds.x - distAmt.x;
	this->maxBounds.y = maxBounds.y - distAmt.y;
}

void Camera::SetFollow(const AEVec2* f, float xOffset, float yOffset, bool setPosToFollow)
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
	position = Easing::SmoothDamp(position, *follow, velocity, smoothTime, dt);

	// === Clamp bounds ===
	if (position.x < minBounds.x)
	{
		velocity.x = 0.f;
		position.x = minBounds.x;
	}
	else if (position.x > maxBounds.x)
	{
		velocity.x = 0.f;
		position.x = maxBounds.x;
	}

	if (position.y < minBounds.y)
	{
		velocity.y = 0.f;
		position.y = minBounds.y;
	}
	else if (position.y > maxBounds.y)
	{
		velocity.y = 0.f;
		position.y = maxBounds.y;
	}
	AEGfxSetCamPosition(position.x * Camera::scale, position.y * Camera::scale);
}
