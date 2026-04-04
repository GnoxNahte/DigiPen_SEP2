/*!
@file		GameOver.h
@author 	Wei Xiang NG
@brief		This header file declares the functions for managing the eyelid 
			effect used in the game over sequence, including building and freeing 
			meshes, updating progress, drawing the eyelid, and checking if the 
			eyelid is fully open or closed. It also includes necessary headers and 
			defines constants for the eyelid animation and behavior.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
#pragma once
void BuildEyelidMeshes();
void FreeEyelidMeshes();
void UpdateEyelid(float dt);
void DrawEyelid();
void ResetEyelid();
bool EyelidDone();
float GetEyelidProgress();

void SetEyelidProgress(float progressPx);
float GetEyelidProgress();
float GetEyelidMaxProgress();
void UpdateEyelidClose(float dt);
void UpdateEyelidOpen(float dt);
void DrawEyelidAtProgress(float progressPx);

bool EyelidFullyClosed();
bool EyelidFullyOpen();

// keep old name for current game-over code
inline void UpdateEyelid(float dt) { UpdateEyelidClose(dt); }
inline bool EyelidDone() { return EyelidFullyClosed(); }