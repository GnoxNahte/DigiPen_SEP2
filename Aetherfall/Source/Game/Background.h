/*!
@file		Background.h
@author 	Wei Xiang NG
@brief		This header file declares the Background class for 
			managing the game's background layers, including loading textures, 
			rendering with parallax effect, and cleaning up resources. The Background 
			class provides static methods for initialization, rendering, and exiting, and 
			it maintains static members for background textures, mesh, and parallax factors
			to create a visually appealing layered background in the game scenes.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
#pragma once
#include "AEEngine.h"
class Background
{
public:
	static void Init();
	static void Render();
	static void Exit();

private:
	static const int UNIQUE_BG_TEXTURES = 3;
	inline static AEGfxTexture* backgroundLayers[UNIQUE_BG_TEXTURES] = { nullptr };
	// Mesh for backgrounds.
	inline static AEGfxVertexList* rectMesh = nullptr;
	// Following sprite dimensions are from the given assets, used for scaling and parallax calculations.
	static const int BACKGROUND_WIDTH = 2048;
	static const int MIDGROUND_WIDTH = 1817;
	static const int FOREGROUND_WIDTH = 1929;
	static const int BACKGROUND_HEIGHT = 400;
	inline static float parallaxFactors[3] = { 0.1f, 0.2f, 0.3f };
};

