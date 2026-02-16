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
	float y = AEGfxGetWindowHeight() - screenPosition.y;
	outWorldPosition.x = (float)(screenPosition.x - AEGfxGetWindowWidth() * 0.5f) / Camera::scale + Camera::position.x;
	outWorldPosition.y = (float)(y - AEGfxGetWindowHeight() * 0.5f) / Camera::scale + Camera::position.y;
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
