#include "Camera.h"
#include "../Utils/AEExtras.h"
#include "../Utils/Easing.h"
#include "../Game/Time.h"
// followed object crosses the current room edge, we shift the roomTarget by
// exactly one screen-sized "roomSize" in that direction.

// Default camera scale
float Camera::scale = 1.f;
AEVec2 Camera::position{ 0.f, 0.f };

Camera::Camera(const AEVec2& minBounds, const AEVec2& maxBounds, float _scale) :
	offset(0, 0),
	follow(nullptr),
	halfView(0, 0),
	roomSize(0, 0),
	roomTarget(0, 0),
	smoothTime(0.2f),
	deadzoneRange(0, 0),
	velocity(0)
{
	Camera::scale = _scale;

	// Top left and middle of the screen in world space
	AEVec2 LeftMiddle, TopMiddle;
	AEExtras::ScreenToWorldPosition({ 0.f, AEGfxGetWindowHeight() * 0.5f }, LeftMiddle);
	AEExtras::ScreenToWorldPosition(
		{ AEGfxGetWindowWidth() * 0.5f, 0.f },
		TopMiddle
	);

	// Distance between the middle and Left
	AEVec2 distAmt;
	AEVec2Sub(&distAmt, &TopMiddle, &LeftMiddle);

	// Save half-view extents and room size in world space
	halfView = distAmt;
	roomSize.x = distAmt.x * 2.f;
	roomSize.y = distAmt.y * 2.f;

	this->minBounds.x = minBounds.x + distAmt.x;
	this->minBounds.y = minBounds.y + distAmt.y;

	this->maxBounds.x = maxBounds.x - distAmt.x;
	this->maxBounds.y = maxBounds.y - distAmt.y;

	// Default room target to the initial camera position.
	roomTarget = position;
}

void Camera::SetFollow(const AEVec2* f, float xOffset, float yOffset, bool setPosToFollow)
{
	this->follow = f;
	this->offset.x = xOffset;
	this->offset.y = yOffset;

	// Room-based start: snap the roomTarget to the room that contains the follow position.

	if (f)
	{
		// Room grid origin is the map min bound (camera-center min minus half view).
		AEVec2 origin{ minBounds.x - halfView.x, minBounds.y - halfView.y };

		int ix = 0;
		int iy = 0;
		if (roomSize.x > 0.f) ix = static_cast<int>((f->x - origin.x) / roomSize.x);
		if (roomSize.y > 0.f) iy = static_cast<int>((f->y - origin.y) / roomSize.y);

		roomTarget.x = minBounds.x + static_cast<float>(ix) * roomSize.x;
		roomTarget.y = minBounds.y + static_cast<float>(iy) * roomSize.y;

		// Clamp room target to allowed camera-center bounds.
		if (roomTarget.x < minBounds.x) roomTarget.x = minBounds.x;
		if (roomTarget.x > maxBounds.x) roomTarget.x = maxBounds.x;
		if (roomTarget.y < minBounds.y) roomTarget.y = minBounds.y;
		if (roomTarget.y > maxBounds.y) roomTarget.y = maxBounds.y;
	}

	if (setPosToFollow)
		position = roomTarget;
}

void Camera::Update()
{
	float dt = static_cast<float>(Time::GetInstance().GetScaledDeltaTime());

	// Safety: if no follow target, just apply current camera position.
	if (!follow)
	{
		AEGfxSetCamPosition(position.x * Camera::scale, position.y * Camera::scale);
		return;
	}

	// === Room stepping ===
	// If the followed object crosses the current room boundary, shift the target room
	// center by exactly one screen-sized "roomSize".
	const AEVec2& p = *follow;
	if (roomSize.x > 0.f)
	{
		while (p.x > roomTarget.x + halfView.x) roomTarget.x += roomSize.x;
		while (p.x < roomTarget.x - halfView.x) roomTarget.x -= roomSize.x;
	}
	if (roomSize.y > 0.f)
	{
		while (p.y > roomTarget.y + halfView.y) roomTarget.y += roomSize.y;
		while (p.y < roomTarget.y - halfView.y) roomTarget.y -= roomSize.y;
	}

	// Clamp the room target so the camera view never shows outside the map bounds.
	if (roomTarget.x < minBounds.x) roomTarget.x = minBounds.x;
	else if (roomTarget.x > maxBounds.x) roomTarget.x = maxBounds.x;
	if (roomTarget.y < minBounds.y) roomTarget.y = minBounds.y;
	else if (roomTarget.y > maxBounds.y) roomTarget.y = maxBounds.y;

	// Smoothly move the camera toward the room center.
	position = Easing::SmoothDamp(position, roomTarget, velocity, smoothTime, dt);

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
