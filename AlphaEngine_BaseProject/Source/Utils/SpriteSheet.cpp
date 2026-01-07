#include <iostream>

#include "SpriteSheet.h"
#include "MeshGenerator.h"

SpriteSheet::SpriteSheet(std::string file, int rows, int cols, float framesPerSecond) : uvOffset(0.f, 0.f), rows(rows), cols(cols)
{
	timePerFrame = 1.f / framesPerSecond;

	if (timePerFrame <= 0.f)
		std::cout << "[WARNING] timePerFrame <= 0.f" << std::endl;

	uvWidth = 1.f / cols;
	uvHeight = 1.f / rows;

	animTimer = 0.f;
	spriteIndex = 0;

	mesh = MeshGenerator::GetRectMesh(50.f, 50.f, uvWidth, uvHeight);
	texture = AEGfxTextureLoad(file.c_str());
}

void SpriteSheet::Update()
{
	if (animTimer >= timePerFrame)
	{
		animTimer -= timePerFrame;

		spriteIndex = (spriteIndex + 1) % cols;
		uvOffset.x = spriteIndex * uvWidth;
		std::cout << "i:" << uvOffset.x << std::endl;
	}

	animTimer += (float)AEFrameRateControllerGetFrameTime();
}

void SpriteSheet::Render()
{
	AEGfxTextureSet(texture, uvOffset.x, uvOffset.y);
	AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
}
