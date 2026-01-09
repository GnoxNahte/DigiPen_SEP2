#include <iostream>

#include "Sprite.h"
#include "MeshGenerator.h"

Sprite::Sprite(std::string file) 
	: uvOffset(0.f, 0.f), metadata(file)
{
	timePerFrame = 1.f / metadata.framesPerSecond;

	if (timePerFrame <= 0.f)
		std::cout << "[WARNING] timePerFrame <= 0.f" << std::endl;

	uvWidth = 1.f / metadata.cols;
	uvHeight = 1.f / metadata.rows;

	animTimer = 0.f;
	spriteIndex = 0;

	mesh = MeshGenerator::GetRectMesh(1.f, 1.f, uvWidth, uvHeight);
	texture = AEGfxTextureLoad(file.c_str());
}

void Sprite::Update()
{
	if (animTimer >= timePerFrame)
	{
		animTimer -= timePerFrame;

		spriteIndex = (spriteIndex + 1) % metadata.cols;
		uvOffset.x = spriteIndex * uvWidth;
	}

	animTimer += (float)AEFrameRateControllerGetFrameTime();
}

void Sprite::Render()
{
	AEGfxTextureSet(texture, uvOffset.x, uvOffset.y);
	AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
	AEGfxMeshDraw(mesh, AE_GFX_MDM_LINES_STRIP);
	AEGfxTextureSet(nullptr, 0, 0); // Reset
}
