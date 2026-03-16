#include "AudioManager.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include "../Game/Time.h"

namespace
{
    const f32 PI_2 = 1.57079632679f;

    bool gAudioInitialized = false;
    bool gAudioLoaded = false;

    float gMasterVolume = 1.0f;
    float gSFXVolume = 1.0f;
    float gMusicVolume = 1.0f;

    AEAudioGroup gSFXGroup{};
    AEAudioGroup gMusicGroup{};

    AEAudioGroup gFadeOutGroup{};
    AEAudioGroup gFadeInGroup{};

    // SFX
    AEAudio gPlayerAttack1{};
    AEAudio gPlayerAttack2{};
    AEAudio gPlayerAttack3{};

    // Music
    AEAudio gMainMenuMusic{};
    AEAudio gGameSceneMusic{};
    //AEAudio gBossIntroMusic{};
    //AEAudio gBossCombatMusic{};
    AEAudio gVictoryMusic{};
    AEAudio gDeathMusic{};

    Audio gBossIntroMusic{};
    Audio gBossCombatMusic{};

    int gNextAttackIndex = 0;

    bool gHasCurrentMusic = false;
    MusicId gCurrentMusicId = MusicId::MainMenu;
    float gCurrentMusicVolumeScale = 1.0f;
    float gCurrentMusicPitch = 1.0f;
    int gCurrentMusicLoopCount = 0;

    // Crossfade variables
    // Static pointers to track which channels are currently fading
    Audio* gFromChannel = nullptr;
    Audio* gToChannel = nullptr;

    f32 gFadeTimer = 0.0f;
    f32 gFadeDuration = 0.0f;
    bool gIsCrossfading = false;

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

    gFadeOutGroup = AEAudioCreateGroup();
    gFadeInGroup = AEAudioCreateGroup();

    gAudioInitialized = true;

    ApplyGroupVolumes();
}

void AudioManager::Update() {
    // Trigger check
    if (AEInputCheckTriggered(AEVK_1)) {
        Crossfade(gBossIntroMusic, gBossCombatMusic, 5.f);
    }
    if (AEInputCheckTriggered(AEVK_2)) {
        Crossfade(gBossCombatMusic, gBossIntroMusic, 4.5f);
    }

    if (gIsCrossfading) {
        gFadeTimer += static_cast<f32>(Time::GetInstance().GetScaledDeltaTime());

        // Safety check for duration to avoid division by zero
        f32 t = (gFadeDuration > 0.0f) ? Clamp01(gFadeTimer / gFadeDuration) : 1.0f;

        if (t >= 1.0f) {
            AEAudioStopGroup(gFadeOutGroup);
            // Set final volume to be precise
            AEAudioSetGroupVolume(gFadeInGroup, 1.0f * gMusicVolume * gMasterVolume);
            gIsCrossfading = false;
        }
        else {
            // Apply curves
            f32 outVol = std::cos(t * PI_2) * gMusicVolume * gMasterVolume;
            f32 inVol = std::sin(t * PI_2) * gMusicVolume * gMasterVolume;
            std::cout << inVol << "    " << outVol << '\n';
            AEAudioSetGroupVolume(gFadeOutGroup, outVol);
            AEAudioSetGroupVolume(gFadeInGroup, inVol);
        }
    }
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
    gBossIntroMusic.audioFile = AEAudioLoadMusic("Assets/music/BossIntro.mp3");
    gBossCombatMusic.audioFile = AEAudioLoadMusic("Assets/music/BossFight.mp3");
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

    if (AEAudioIsValidGroup(gFadeOutGroup)) 
        AEAudioUnloadAudioGroup(gFadeOutGroup);
    if (AEAudioIsValidGroup(gFadeInGroup)) 
        AEAudioUnloadAudioGroup(gFadeInGroup);

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
    if (AEAudioIsValidAudio(gBossIntroMusic.audioFile))
        AEAudioUnloadAudio(gBossIntroMusic.audioFile);
    if (AEAudioIsValidAudio(gBossCombatMusic.audioFile))
        AEAudioUnloadAudio(gBossCombatMusic.audioFile);
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
    gBossIntroMusic.audioFile = AEAudio{};
    gBossCombatMusic.audioFile = AEAudio{};
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
void AudioManager::Crossfade(Audio& from, Audio& to, f32 duration) {
    if (!gAudioLoaded) LoadAll();

    // 1. Reset everything
    gFadeTimer = 0.0f;
    gFadeDuration = duration;
    gFromChannel = &from;
    gToChannel = &to;
    gIsCrossfading = true;

    // 2. Set INITIAL volumes immediately (t = 0)
    // cos(0) = 1.0, sin(0) = 0.0
    AEAudioSetGroupVolume(gFadeOutGroup, 1.0f * gMusicVolume * gMasterVolume);
    AEAudioSetGroupVolume(gFadeInGroup, 0.0f);

    // 3. Play
    AEAudioPlay(from.audioFile, gFadeOutGroup, 1.0f, 1.0f, -1);
    AEAudioPlay(to.audioFile, gFadeInGroup, 1.0f, 1.0f, -1);
}