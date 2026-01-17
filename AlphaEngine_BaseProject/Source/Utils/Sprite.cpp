#include <iostream>

#include "Sprite.h"
#include "MeshGenerator.h"

Sprite::Sprite(std::string file) 
	: uvOffset(0.f, 0.f), metadata(file)
{
	uvWidth = 1.f / metadata.cols;
	uvHeight = 1.f / metadata.rows;

	animTimer = 0.f;
	currStateIndex = nextStateIndex = 0;
	frameIndex = 0;

	mesh = MeshGenerator::GetRectMesh(1.f, 1.f, uvWidth, uvHeight);
	texture = AEGfxTextureLoad(file.c_str());
}

void Sprite::Update()
{
	const auto& currState = metadata.stateInfoRows[currStateIndex];
	if (animTimer >= currState.timePerFrame)
	{
		animTimer -= currState.timePerFrame;

		// If should change state
		if (currStateIndex != nextStateIndex && 
			(!ifLockCurrent || frameIndex == currState.frameCount - 1))
		{
			animTimer = 0;
			frameIndex = 0;
			currStateIndex = nextStateIndex;
			ifLockCurrent = false;
		}
		else 
			frameIndex = (frameIndex + 1) % currState.frameCount;

		uvOffset.x = frameIndex * uvWidth;
		uvOffset.y = currStateIndex * uvHeight;
		//std::cout << frameIndex << std::endl;
	}

	animTimer += (float)AEFrameRateControllerGetFrameTime();
}

void Sprite::Render()
{
	AEGfxTextureSet(texture, uvOffset.x, uvOffset.y);
	AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
	//AEGfxMeshDraw(mesh, AE_GFX_MDM_LINES_STRIP);
	//AEGfxTextureSet(nullptr, 0, 0); // Reset
}

// todo - Currently it changes immediately. Add condition to only transition when current anim is done?
void Sprite::SetState(int nextState, bool ifLock)
{
	if (nextState == currStateIndex)
		return;

	if (nextState < 0 || nextState > metadata.rows)
	{
		std::cout << "[ERROR] Invalid sprite state" << std::endl;
		return;
	}

	this->nextStateIndex = nextState;
	
	if (ifLockCurrent)
		return;
	
	this->ifLockCurrent = ifLock;

	animTimer = 0;
	currStateIndex = nextState;
	frameIndex = 0;

	uvOffset.x = frameIndex * uvWidth;
	uvOffset.y = currStateIndex * uvHeight;
}
