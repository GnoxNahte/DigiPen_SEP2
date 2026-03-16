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