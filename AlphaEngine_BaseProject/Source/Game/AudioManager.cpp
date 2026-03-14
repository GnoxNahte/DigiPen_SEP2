#include "AudioManager.h"
#include <algorithm>

namespace
{
    bool gAudioInitialized = false;
    bool gAudioLoaded = false;

    float gMasterVolume = 1.0f;
    float gSFXVolume = 1.0f;

    AEAudioGroup gSFXGroup{};
    AEAudio gPlayerDeath{};

    float Clamp01(float v)
    {
        return (std::max)(0.0f, (std::min)(1.0f, v));
    }

    float FinalSFXVolume(float scale)
    {
        return Clamp01(gMasterVolume) * Clamp01(gSFXVolume) * Clamp01(scale);
    }
}

void AudioManager::Init()
{
    if (gAudioInitialized)
        return;

    gSFXGroup = AEAudioCreateGroup();
    gAudioInitialized = true;
}

void AudioManager::LoadAll()
{
    if (!gAudioInitialized)
        Init();

    if (gAudioLoaded)
        return;

    gPlayerDeath = AEAudioLoadSound("Assets/Audio/death.mp3");

    gAudioLoaded = true;
}

void AudioManager::Exit()
{
    if (gAudioLoaded)
    {
        AEUnloadAudio(gPlayerDeath);
        gAudioLoaded = false;
    }

    if (gAudioInitialized)
    {
        AEUnloadAudioGroup(gSFXGroup);
        gAudioInitialized = false;
    }
}

void AudioManager::SetMasterVolume(float v)
{
    gMasterVolume = Clamp01(v);
}

void AudioManager::SetSFXVolume(float v)
{
    gSFXVolume = Clamp01(v);
}

float AudioManager::GetMasterVolume()
{
    return gMasterVolume;
}

float AudioManager::GetSFXVolume()
{
    return gSFXVolume;
}

void AudioManager::PlaySFX(SoundId id, float volumeScale, float pitch, int loopCount)
{
    if (!gAudioLoaded)
        LoadAll();

    AEAudio audio{};

    switch (id)
    {
    case SoundId::PlayerDeath:
        audio = gPlayerDeath;
        break;
    default:
        return;
    }

    AEAudioPlay(audio, gSFXGroup, FinalSFXVolume(volumeScale), pitch, loopCount);
}