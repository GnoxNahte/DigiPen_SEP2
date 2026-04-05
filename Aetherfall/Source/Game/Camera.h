/*!
@file	Camera.h
@author	Ethan Ong, Kang Ping
@brief	Declares a Camera class. Follows a position (mainly player) tracking which rooms they go to
		Stores a camera scale that everything that renders to world-space references

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/

#pragma once
#include "AEEngine.h"
#include "../Editor/EditorUtils.h"

class Camera : Inspectable
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
	~Camera();

	void SetFollow(const AEVec2* follow, float xOffset, float yOffset, bool setPosToFollow);

	void Update();
	
	void SetRoomTarget(const AEVec2& target) { roomTarget = target; }

	static void StartShake(float duration, float magnitude, float frequency = 28.0f);

	// Inherited via Inspectable
	void DrawInspector() override;

private:
	static float shakeTimeLeft;
	static float shakeDuration;
	static float shakeMagnitude;
	static float shakeFrequency;
	static float shakePhase;
};
