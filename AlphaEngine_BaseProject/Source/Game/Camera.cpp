#include "Camera.h"
#include <imgui.h>

#include "../Utils/AEExtras.h"
#include "../Utils/Easing.h"
#include "../Game/Time.h"
#include "../Editor/Editor.h"
#include <cmath>
#include <algorithm>
// followed object crosses the current room edge, we shift the roomTarget by
// exactly one screen-sized "roomSize" in that direction.


//static members for camera shakeee
float Camera::shakeTimeLeft = 0.0f;
float Camera::shakeDuration = 0.0f;
float Camera::shakeMagnitude = 0.0f;
float Camera::shakeFrequency = 28.0f;
float Camera::shakePhase = 0.0f;

// Default camera scale
float Camera::scale = 1.f;
AEVec2 Camera::position{ 0.f, 0.f };

Camera::Camera(const AEVec2& minBounds, const AEVec2& maxBounds, float _scale) :
	Inspectable(true),
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

	Editor::RegisterSystem("Camera", this);
}

Camera::~Camera()
{
	Editor::UnregisterSystem("Camera", this);
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

void Camera::StartShake(float duration, float magnitude, float frequency)
{
	if (duration <= 0.0f || magnitude <= 0.0f)
		return;

	shakeDuration = duration;
	shakeTimeLeft = duration;
	shakeMagnitude = (std::max)(shakeMagnitude, magnitude);
	shakeFrequency = frequency;
	shakePhase = 0.0f;
}

void Camera::Update()
{
	float dt = static_cast<float>(Time::GetInstance().GetScaledDeltaTime());

	// Base camera position from existing room camera logic.
	if (follow)
	{
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

		if (roomTarget.x < minBounds.x) roomTarget.x = minBounds.x;
		else if (roomTarget.x > maxBounds.x) roomTarget.x = maxBounds.x;

		if (roomTarget.y < minBounds.y) roomTarget.y = minBounds.y;
		else if (roomTarget.y > maxBounds.y) roomTarget.y = maxBounds.y;

		position = Easing::SmoothDamp(position, roomTarget, velocity, smoothTime, dt);

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
	}

	// Shake is render-only. Do not modify Camera::position itself.
	AEVec2 finalPos = position;

	if (shakeTimeLeft > 0.0f)
	{

		shakeTimeLeft -= dt;
		if (shakeTimeLeft < 0.0f)
			shakeTimeLeft = 0.0f;

		shakePhase += dt * shakeFrequency * 6.2831853f; //2*pi

		const float t = (shakeDuration > 0.0f) ? (shakeTimeLeft / shakeDuration) : 0.0f;
		const float falloff = t; //do t * t  for a more natural/softer fade
		const float amp = shakeMagnitude * falloff;

		finalPos.x += std::sin(shakePhase) * amp; 
		finalPos.y += std::cos(shakePhase * 1.37f) * amp * 0.45f; //changes vertical shake

		if (shakeTimeLeft <= 0.0f)
		{
			shakeMagnitude = 0.0f;
			shakePhase = 0.0f;
		}
	}

	AEGfxSetCamPosition(finalPos.x * Camera::scale, finalPos.y * Camera::scale);
}

void Camera::DrawInspector()
{
	ImGui::Begin("Camera", &isInspectorOpen);

	ImGui::DragFloat("Scale", &scale, 0.1f);

	ImGui::End();
}
