/*!
@file	Sprite.h
@author	Ethan Ong
@brief	Defines Sprite class which handles sprite animation, linked with sprite metadata
@link	https://gnoxnahte.github.io/DigiPen_SEP2/sprite_usage.html

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
#include "Sprite.h"

#include <iostream>

#include "MeshGenerator.h"
#include "../Game/Time.h"

Sprite::Sprite(const char* _file) :
	file(_file),
	uvOffset(0.f, 0.f), 
	metadata(_file)
{
	uvWidth = 1.f / metadata.cols;
	uvHeight = 1.f / metadata.rows;
	
	ifLockCurrent = false;
	animTimer = 0.f;
	currStateIndex = nextStateIndex = 0;
	frameIndex = 0;

	mesh = MeshGenerator::GetRectMesh(1.f, 1.f, uvWidth, uvHeight);
	texture = AEGfxTextureLoad(_file);
}

Sprite::~Sprite()
{
	AEGfxMeshFree(mesh);
	AEGfxTextureUnload(texture);
}

void Sprite::Update()
{
	const auto& currState = metadata.stateInfoRows[currStateIndex];
	if (animTimer >= currState.timePerFrame)
	{
		animTimer -= currState.timePerFrame;

		bool onLastFrame = frameIndex == (currState.frameCount - 1);

		// If should change state
		if (currStateIndex != nextStateIndex && 
			(!ifLockCurrent || onLastFrame))
		{
			animTimer = 0;
			frameIndex = 0;
			currStateIndex = nextStateIndex;
			ifLockCurrent = false;
		}
		// Else update current animation
		// - if ifLoop, keep repeating
		// - else, play until it's not the last frame
		else if (currState.ifLoop || frameIndex != currState.frameCount - 1)
			frameIndex = (frameIndex + 1) % currState.frameCount;
		
		uvOffset.x = frameIndex * uvWidth;
		uvOffset.y = currStateIndex * uvHeight;

		if (onLastFrame && onAnimEnd)
		{
			onAnimEnd(currStateIndex);
		}
	}

	animTimer += static_cast<float>(Time::GetInstance().GetScaledDeltaTime());
}

void Sprite::Render()
{
	AEGfxTextureSet(texture, uvOffset.x, uvOffset.y);
	AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
	//AEGfxMeshDraw(mesh, AE_GFX_MDM_LINES_STRIP);
	//AEGfxTextureSet(nullptr, 0, 0); // Reset
}

int Sprite::GetState() const
{
	return currStateIndex;
}

void Sprite::SetState(int nextState, bool ifLock, std::function<void(int)> _onAnimEnd)
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

	//std::cout << "Set state: " << nextState << "\n";
	
	this->ifLockCurrent = ifLock;

	this->onAnimEnd = _onAnimEnd;

	animTimer = 0;
	currStateIndex = nextState;
	frameIndex = 0;

	uvOffset.x = frameIndex * uvWidth;
	uvOffset.y = currStateIndex * uvHeight;
}

const char* Sprite::GetFile() const
{
	return file;
}

AEVec2 Sprite::GetUV_Size() const
{
	return { uvWidth, uvHeight };
}
