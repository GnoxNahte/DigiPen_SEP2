#include "AEEngine.h"

#pragma once
class SplashScreen
{
public:
	void Init();
	void Update();
	void Exit();

private:
	// Mesh for black rectangle overlay.
	inline static AEGfxVertexList* rectMesh = nullptr;
	// Font for printing icon
	inline static s8 cardBuffFont;
};

