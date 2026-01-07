#pragma once
#include "AEEngine.h"

// ===== Shapes =====
// Get a Rectangle Mesh with a width and height
// Pivot is in the center
AEGfxVertexList* GetRectMesh(f32 width, f32 height, u32 color);

// Get a Circle Mesh with a radius
// Pivot is in the center
// TODO: Just create 1 circle mesh? Store scale in transform?
AEGfxVertexList* GetCircleMesh(f32 radius, u32 color, int vertexCount = 32);