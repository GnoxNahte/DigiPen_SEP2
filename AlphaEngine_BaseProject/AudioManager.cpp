#include "AudioManager.h"
#include <algorithm>

namespace
{
    bool gAudioInitialized = false;
    bool gAudioLoaded = false;

    float gMasterVolume = 1.0f;
    float gSFXVolume = 1.0f;
    float gMusicVolume = 1.0f;

    AEAudioGroup gSFXGroup{};
    AEAudioGroup gMusicGroup{};

    AEAudio gPlayerDeath{};
    AEAudio gMainMenu_Credit_Music{};
	AEAudio gGamesense{};
    AEAudio gVictory{};

    float Clamp01(float v)
    {
        return (std::max)(0.0f, (std::min)(1.0f, v));
    }

    float FinalSFXVolume(float scale)
    {
        return Clamp01(gMasterVolume) * Clamp01(gSFXVolume) * Clamp01(scale);
    }

    float FinalMusicVolume(float scale)
    {
        return Clamp01(gMasterVolume) * Clamp01(gMusicVolume) * Clamp01(scale);
    }
}

void AudioManager::Init()
{
    if (gAudioInitialized)
        return;

    gSFXGroup = AEAudioCreateGroup();
    gMusicGroup = AEAudioCreateGroup();
    gAudioInitialized = true;
}

void AudioManager::LoadAll()
{
    if (!gAudioInitialized)
        Init();

    if (gAudioLoaded)
        return;

    gPlayerDeath = AEAudioLoadSound("Assets/music/death.mp3");
    gMainMenu_Credit_Music = AEAudioLoadMusic("Assets/music/mainmenu.mp3");
	gGamesense = AEAudioLoadMusic("Assets/music/gamesense.mp3");
	gVictory = AEAudioLoadMusic("Assets/music/victory.mp3");


    gAudioLoaded = true;
}

void AudioManager::Exit()
{
    // 你这版 AlphaEngine 没有 AEUnloadAudio / AEUnloadAudioGroup
    // 所以第一阶段先不要卸载，先保证能编译、能播放
}

void AudioManager::SetMasterVolume(float v)
{
    gMasterVolume = Clamp01(v);
}

void AudioManager::SetSFXVolume(float v)
{
    gSFXVolume = Clamp01(v);
}

void AudioManager::SetMusicVolume(float v)
{
    gMusicVolume = Clamp01(v);
}

float AudioManager::GetMasterVolume()
{
    return gMasterVolume;
}

float AudioManager::GetSFXVolume()
{
    return gSFXVolume;
}

float AudioManager::GetMusicVolume()
{
    return gMusicVolume;
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

void AudioManager::PlayMusic(MusicId id, float volumeScale, float pitch, int loopCount)
{
    if (!gAudioLoaded)
        LoadAll();

    AEAudio audio{};

    switch (id)
    {
    case MusicId::MainMenu:
        audio = gMainMenu_Credit_Music;
        break;
    default:
        return;
    }

    AEAudioPlay(audio, gMusicGroup, FinalMusicVolume(volumeScale), pitch, loopCount);
}