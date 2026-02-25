#pragma once
#include "AEEngine.h"
class Background
{
public:
	static void Init();
	static void Render();

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
	inline static float parallaxFactors[3] = { 0.1f, 0.4f, 0.8f };
};

