#include "AEExtras.h"
#include "../Game/Camera.h"

void AEExtras::GetCursorWorldPosition(AEVec2& outPosition, const AEVec2& cameraPosition)
{
	s32 mousePosX, mousePosY;
	AEInputGetCursorPosition(&mousePosX, &mousePosY);
	outPosition.x = (float)(mousePosX - AEGfxGetWindowWidth() * 0.5f) / Camera::scale + cameraPosition.x;
	outPosition.y = (float)(mousePosY - AEGfxGetWindowHeight() * 0.5f) / Camera::scale + cameraPosition.y;
}

void AEExtras::ScreenToWorldPosition(const AEVec2& screenPosition, const AEVec2& cameraPosition, AEVec2& outWorldPosition)
{
	outWorldPosition.x = (float)(screenPosition.x - AEGfxGetWindowWidth() * 0.5f) / Camera::scale + cameraPosition.x;
	outWorldPosition.y = (float)(screenPosition.y - AEGfxGetWindowHeight() * 0.5f) / Camera::scale + cameraPosition.y;
}
