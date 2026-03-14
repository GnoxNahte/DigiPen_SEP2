#pragma once
#include "AEEngine.h"

enum class SoundId
{
    PlayerDeath
};

class AudioManager
{
public:
    static void Init();
    static void LoadAll();
    static void Exit();

    static void SetMasterVolume(float v);
    static void SetSFXVolume(float v);

    static float GetMasterVolume();
    static float GetSFXVolume();

    static void PlaySFX(SoundId id, float volumeScale = 1.0f, float pitch = 1.0f, int loopCount = 0);
};