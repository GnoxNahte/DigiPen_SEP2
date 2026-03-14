#pragma once
#include "AEEngine.h"

enum class SoundId
{
    PlayerAttack1,
    PlayerAttack2,
    PlayerAttack3
};

enum class MusicId
{
    MainMenu,
    Death
};

class AudioManager
{
public:
    static void Init();
    static void LoadAll();
    static void Exit();

    static void SetMasterVolume(float v);
    static void SetSFXVolume(float v);
    static void SetMusicVolume(float v);

    static float GetMasterVolume();
    static float GetSFXVolume();
    static float GetMusicVolume();

    static void PlaySFX(SoundId id, float volumeScale = 1.0f, float pitch = 1.0f, int loopCount = 0);
    static void PlayMusic(MusicId id, float volumeScale = 1.0f, float pitch = 1.0f, int loopCount = 0);

    static void PlayNextAttackSFX();
};