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
    AEAudio gGameSceneMusic{};
    AEAudio gVictoryMusic{};
    AEAudio gDeathMusic{};

    int gNextAttackIndex = 0;

    bool gHasCurrentMusic = false;
    MusicId gCurrentMusicId = MusicId::MainMenu;
    float gCurrentMusicVolumeScale = 1.0f;
    float gCurrentMusicPitch = 1.0f;
    int gCurrentMusicLoopCount = 0;

    float Clamp01(float v)
    {
        return (std::max)(0.0f, (std::min)(1.0f, v));
    }

    float FinalSFXVolume(float scale)
    {
        return Clamp01(scale);
    }

    float FinalMusicVolume(float scale)
    {
        return Clamp01(scale);
    }

    void RefreshCurrentMusicState(MusicId id, float volumeScale, float pitch, int loopCount)
    {
        gCurrentMusicId = id;
        gCurrentMusicVolumeScale = volumeScale;
        gCurrentMusicPitch = pitch;
        gCurrentMusicLoopCount = loopCount;
        gHasCurrentMusic = true;
    }

    void ApplyGroupVolumes()
    {
        const float sfxGroupVol = Clamp01(gMasterVolume) * Clamp01(gSFXVolume);
        const float musicGroupVol = Clamp01(gMasterVolume) * Clamp01(gMusicVolume);

        if (AEAudioIsValidGroup(gSFXGroup))
            AEAudioSetGroupVolume(gSFXGroup, sfxGroupVol);

        if (AEAudioIsValidGroup(gMusicGroup))
            AEAudioSetGroupVolume(gMusicGroup, musicGroupVol);
    }
}

void AudioManager::Init()
{
    if (gAudioInitialized)
        return;

    gSFXGroup = AEAudioCreateGroup();
    gMusicGroup = AEAudioCreateGroup();
    gAudioInitialized = true;

    ApplyGroupVolumes();
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
    //gMainMenuMusic = AEAudioLoadMusic("Assets/music/mainmenu.mp3");
    gMainMenuMusic = AEAudioLoadMusic("Assets/music/MenuBGM.mp3");
    gDeathMusic = AEAudioLoadMusic("Assets/music/Defeat.mp3");
    gGameSceneMusic = AEAudioLoadMusic("Assets/music/GameBGM.mp3");
    gVictoryMusic = AEAudioLoadMusic("Assets/music/victory.mp3");

    gAudioLoaded = true;
}

void AudioManager::Exit()
{
	// stop all audio first
    if (AEAudioIsValidGroup(gSFXGroup))
        AEAudioStopGroup(gSFXGroup);

    if (AEAudioIsValidGroup(gMusicGroup))
        AEAudioStopGroup(gMusicGroup);

	// unload SFX
    if (AEAudioIsValidAudio(gPlayerAttack1))
        AEAudioUnloadAudio(gPlayerAttack1);
    if (AEAudioIsValidAudio(gPlayerAttack2))
        AEAudioUnloadAudio(gPlayerAttack2);
    if (AEAudioIsValidAudio(gPlayerAttack3))
        AEAudioUnloadAudio(gPlayerAttack3);

    // unload Music
    if (AEAudioIsValidAudio(gMainMenuMusic))
        AEAudioUnloadAudio(gMainMenuMusic);
    if (AEAudioIsValidAudio(gDeathMusic))
        AEAudioUnloadAudio(gDeathMusic);
    if (AEAudioIsValidAudio(gGameSceneMusic))
        AEAudioUnloadAudio(gGameSceneMusic);
    if (AEAudioIsValidAudio(gVictoryMusic))
        AEAudioUnloadAudio(gVictoryMusic);

    // unload group
    if (AEAudioIsValidGroup(gSFXGroup))
        AEAudioUnloadAudioGroup(gSFXGroup);
    if (AEAudioIsValidGroup(gMusicGroup))
        AEAudioUnloadAudioGroup(gMusicGroup);

	// cleaning up state
    gPlayerAttack1 = AEAudio{};
    gPlayerAttack2 = AEAudio{};
    gPlayerAttack3 = AEAudio{};

    gMainMenuMusic = AEAudio{};
    gDeathMusic = AEAudio{};
    gGameSceneMusic = AEAudio{};
    gVictoryMusic = AEAudio{};

    gSFXGroup = AEAudioGroup{};
    gMusicGroup = AEAudioGroup{};

    gNextAttackIndex = 0;

    gHasCurrentMusic = false;
    gCurrentMusicId = MusicId::MainMenu;
    gCurrentMusicVolumeScale = 1.0f;
    gCurrentMusicPitch = 1.0f;
    gCurrentMusicLoopCount = 0;

    gAudioLoaded = false;
    gAudioInitialized = false;

    gMasterVolume = 1.0f;
    gSFXVolume = 1.0f;
    gMusicVolume = 1.0f;
}

void AudioManager::SetMasterVolume(float v)
{
    gMasterVolume = Clamp01(v);
    ApplyGroupVolumes();
}

void AudioManager::SetSFXVolume(float v)
{
    gSFXVolume = Clamp01(v);
    ApplyGroupVolumes();
}

void AudioManager::SetMusicVolume(float v)
{
    gMusicVolume = Clamp01(v);
    ApplyGroupVolumes();
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

    ApplyGroupVolumes();
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
    case MusicId::GameScene:
        audio = gGameSceneMusic;
        break;
    case MusicId::Victory:
        audio = gVictoryMusic;
        break;
    default:
        return;
    }

    RefreshCurrentMusicState(id, volumeScale, pitch, loopCount);
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
    default:
        break;
    }

    gNextAttackIndex = (gNextAttackIndex + 1) % 3;
}