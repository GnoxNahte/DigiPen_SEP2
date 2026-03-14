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

    // SFX
    AEAudio gPlayerAttack1{};
    AEAudio gPlayerAttack2{};
    AEAudio gPlayerAttack3{};

    // Music
    AEAudio gMainMenuMusic{};
    AEAudio gDeathMusic{};

	// attack loop index for player attack SFX, will loop through 3 different attack SFX in order to add variety to the sound effects when player attacks
    int gNextAttackIndex = 0;

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

    // SFX
    gPlayerAttack1 = AEAudioLoadSound("Assets/music/player_attack1.mp3");
    gPlayerAttack2 = AEAudioLoadSound("Assets/music/player_attack2.mp3");
    gPlayerAttack3 = AEAudioLoadSound("Assets/music/player_attack3.mp3");

    // Music
    gMainMenuMusic = AEAudioLoadMusic("Assets/music/mainmenu.mp3");
    gDeathMusic = AEAudioLoadMusic("Assets/music/death.mp3");

    gAudioLoaded = true;
}

void AudioManager::Exit()
{
    gPlayerAttack1 = AEAudio{};
    gPlayerAttack2 = AEAudio{};
    gPlayerAttack3 = AEAudio{};

    gMainMenuMusic = AEAudio{};
    gDeathMusic = AEAudio{};

    gSFXGroup = AEAudioGroup{};
    gMusicGroup = AEAudioGroup{};

    gNextAttackIndex = 0;

    gAudioLoaded = false;
    gAudioInitialized = false;

    gMasterVolume = 1.0f;
    gSFXVolume = 1.0f;
    gMusicVolume = 1.0f;
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
    case SoundId::PlayerAttack1:
        audio = gPlayerAttack1;
        break;
    case SoundId::PlayerAttack2:
        audio = gPlayerAttack2;
        break;
    case SoundId::PlayerAttack3:
        audio = gPlayerAttack3;
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
        audio = gMainMenuMusic;
        break;
    case MusicId::Death:
        audio = gDeathMusic;
        break;
    default:
        return;
    }

    AEAudioPlay(audio, gMusicGroup, FinalMusicVolume(volumeScale), pitch, loopCount);
}

void AudioManager::PlayNextAttackSFX()
{
    switch (gNextAttackIndex)
    {
    case 0:
        PlaySFX(SoundId::PlayerAttack1);
        break;
    case 1:
        PlaySFX(SoundId::PlayerAttack2);
        break;
    case 2:
        PlaySFX(SoundId::PlayerAttack3);
        break;
    }

    gNextAttackIndex = (gNextAttackIndex + 1) % 3;
}