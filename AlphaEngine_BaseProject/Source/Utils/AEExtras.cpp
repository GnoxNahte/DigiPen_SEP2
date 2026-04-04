/*!
@file	AEExtras.cpp
@author	Ethan Ong
@brief	Defines functions to extend Alpha Engine's functionality
		Also overloads AEVec2 for easier AEVec2 usage and better readability

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
#include "AEExtras.h"
#include "../Game/Camera.h"

void AEExtras::GetCursorWorldPosition(AEVec2& outPosition)
{
	s32 mousePosX, mousePosY;
	AEInputGetCursorPosition(&mousePosX, &mousePosY);
	ScreenToWorldPosition({ (float)mousePosX, (float)mousePosY }, outPosition);
}

void AEExtras::ScreenToWorldPosition(const AEVec2& screenPosition, AEVec2& outWorldPosition)
{
	outWorldPosition.x = ( screenPosition.x - AEGfxGetWindowWidth()  * 0.5f) / Camera::scale + Camera::position.x;
	outWorldPosition.y = (-screenPosition.y + AEGfxGetWindowHeight() * 0.5f) / Camera::scale + Camera::position.y;
}

void AEExtras::WorldToScreenPosition(const AEVec2& worldPosition, AEVec2& outScreenPosition)
{
	outScreenPosition.x =  (worldPosition.x - Camera::position.x) * Camera::scale + AEGfxGetWindowWidth() * 0.5f;
	outScreenPosition.y = -(worldPosition.y - Camera::position.y) * Camera::scale + AEGfxGetWindowHeight() * 0.5f;
}

void AEExtras::WorldToViewportPosition(const AEVec2& worldPosition, AEVec2& outViewportPosition)
{
	WorldToScreenPosition(worldPosition, outViewportPosition);
	outViewportPosition.x /= AEGfxGetWindowWidth();
	outViewportPosition.y = 1 - outViewportPosition.y / AEGfxGetWindowHeight();
}

void AEExtras::WorldToOpenGL_Coords(const AEVec2& worldPosition, AEVec2& outputPosition)
{
	WorldToScreenPosition(worldPosition, outputPosition);
	outputPosition.x =  (outputPosition.x / AEGfxGetWindowWidth()) * 2.f - 1.f;
	outputPosition.y = -((outputPosition.y / AEGfxGetWindowHeight()) * 2.f - 1.f);
}

float AEExtras::RandomRange(const AEVec2& range)
{
	return AERandFloat() * (range.y - range.x) + range.x;
}

float AEExtras::Remap(float value, const AEVec2& inRange, const AEVec2& outRange)
{
	return (value - inRange.x) / (inRange.y - inRange.x) * (outRange.y - outRange.x) + outRange.x;
}

float AEExtras::RemapClamp(float value, const AEVec2& inRange, const AEVec2& outRange)
{
	return AEClamp(Remap(value, inRange, outRange), outRange.x, outRange.y);
}
