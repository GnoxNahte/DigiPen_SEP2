#include "AudioManager.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include "../Game/Time.h"

BGMAudio::BGMAudio(char const* filename)  // Ctor for BGMAudio. Default ctor is disallowed.
    : volume{ 1.0f }, pitch{ 1.0f }, active{ false } // Initialize variables
{
    audioFile = AEAudioLoadMusic(filename); // Load file
    ownGroup = AEAudioCreateGroup();  // own group, so we can SetVolume independently
    if (!AEAudioIsValidAudio(audioFile)) {
        std::cout << "Failed to load audio: " << filename << "\n";
}

}
BGMAudio::~BGMAudio() { // Dtor for automatic cleanup
    if (AEAudioIsValidGroup(ownGroup)) {
        AEAudioStopGroup(ownGroup);
        AEAudioUnloadAudioGroup(ownGroup);
        ownGroup = {};
    }
    if (AEAudioIsValidAudio(audioFile)) {
        AEAudioUnloadAudio(audioFile);
        audioFile = {};
    }
}




namespace
{
    const f32 PI_2 = 1.57079632679f;

    //bool gAudioInitialized = false;
    //bool gAudioLoaded = false;

    float gMasterVolume = 1.0f;
    float gSFXVolume = 1.0f;
    float gMusicVolume = 1.0f;

    AEAudioGroup gSFXGroup{};
    AEAudioGroup gMusicGroup{};

    std::unique_ptr<BGMAudio> bossIntroMusic;
    std::unique_ptr<BGMAudio> bossFightMusic;

    //bool gHasCurrentMusic = false;
    //MusicId gCurrentMusicId = MusicId::MainMenu;
    //float gCurrentMusicVolumeScale = 1.0f;
    //float gCurrentMusicPitch = 1.0f;
    //int gCurrentMusicLoopCount = 0;

    //// Static pointers to track which channels are currently fading
    //Audio* gFromChannel = nullptr;
    //Audio* gToChannel = nullptr;

        //// Crossfade variables
    f32 gFadeTimer = 0.0f;
    f32 gFadeDuration = 0.0f;
    bool gIsCrossfading = false;
    BGMAudio* gFadeOutTrack = nullptr;
    BGMAudio* gFadeInTrack = nullptr;

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

    //void RefreshCurrentMusicState(MusicId id, float volumeScale, float pitch, int loopCount)
    //{
    //    gCurrentMusicId = id;
    //    gCurrentMusicVolumeScale = volumeScale;
    //    gCurrentMusicPitch = pitch;
    //    gCurrentMusicLoopCount = loopCount;
    //    gHasCurrentMusic = true;
    //}

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
void BGMAudio::Play(f32 const& vol) {
    volume = vol;
    AEAudioSetGroupVolume(ownGroup, volume * gMusicVolume * gMasterVolume);
    AEAudioPlay(audioFile, ownGroup, vol, pitch, -1);
}
void BGMAudio::Stop() {
    if (AEAudioIsValidGroup(ownGroup))
        AEAudioStopGroup(ownGroup);
}
void BGMAudio::SetVolume(f32 const& vol) {
    volume = vol;
    if (AEAudioIsValidGroup(ownGroup))
        AEAudioSetGroupVolume(ownGroup, vol * gMusicVolume * gMasterVolume);
}
void BGMAudio::CrossfadeTo(BGMAudio& other, f32 duration) {
    gFadeOutTrack = this;
    gFadeInTrack = &other;
    gFadeTimer = 0.0f;
    gFadeDuration = duration;
    gIsCrossfading = true;

    // Only play if it's not already running silently
    // Note: You may need a bool "isPlaying" inside BGMAudio to track this
    if (!other.active) {
        AEAudioPlay(other.audioFile, other.ownGroup, 1.0f, other.pitch, -1);
        other.active = true;
    }
}
void AudioManager::Init() {
    if (!AEAudioIsValidGroup(gMusicGroup)) {
        gMusicGroup = AEAudioCreateGroup();
    }
    if (!AEAudioIsValidGroup(gSFXGroup)) {
        gSFXGroup = AEAudioCreateGroup();
    }
    ApplyGroupVolumes(); // Ensure the group volume is applied

    // Load BGMs here.
    if (!bossIntroMusic)
        bossIntroMusic = std::make_unique<BGMAudio>("Assets/music/BossIntro.mp3");
    if (!bossFightMusic)
        bossFightMusic = std::make_unique<BGMAudio>("Assets/music/BossFight.mp3");
    bossIntroMusic->Play(bossIntroMusic->GetVolume());
    bossIntroMusic->SetActive(true);
    bossFightMusic->Play(0.0f);
}

void AudioManager::Update() {
    if (AEInputCheckTriggered(AEVK_1)) {
        bossFightMusic->CrossfadeTo(*bossIntroMusic, 0.8f);
    }
    if (AEInputCheckTriggered(AEVK_2)) {
        bossIntroMusic->CrossfadeTo(*bossFightMusic, 0.8f);
    }
    if (gIsCrossfading && gFadeOutTrack && gFadeInTrack) {
        gFadeTimer += static_cast<f32>(Time::GetInstance().GetScaledDeltaTime());
        f32 t = (gFadeDuration > 0.0f) ? Clamp01(gFadeTimer / gFadeDuration) : 1.0f;

        std::cout << "t=" << t << " out=" << std::cos(t * PI_2) << " in=" << std::sin(t * PI_2) << "\n";

        if (t >= 1.0f) {
            //gFadeOutTrack->Stop();
            gFadeOutTrack->SetVolume(0.0f);
            gFadeInTrack->SetVolume(1.0f);
            gIsCrossfading = false;
            gFadeOutTrack = nullptr;
            gFadeInTrack = nullptr;
        }
        else {
            gFadeOutTrack->SetVolume(std::cos(t * PI_2));
            gFadeInTrack->SetVolume(std::sin(t * PI_2));
        }
    }
}
void AudioManager::Exit() {
    bossIntroMusic.reset();
    bossFightMusic.reset();
}

//void AudioManager::Init()
//{
//    if (gAudioInitialized)
//        return;
//
//    gSFXGroup = AEAudioCreateGroup();
//    gMusicGroup = AEAudioCreateGroup();
//
//    gFadeOutGroup = AEAudioCreateGroup();
//    gFadeInGroup = AEAudioCreateGroup();
//
//    gAudioInitialized = true;
//
//    ApplyGroupVolumes();
//}
//
//void AudioManager::Update() {
//    //// Trigger check
//    //if (AEInputCheckTriggered(AEVK_1)) {
//    //    Crossfade(gBossIntroMusic, gBossCombatMusic, 5.f);
//    //}
//    //if (AEInputCheckTriggered(AEVK_2)) {
//    //    Crossfade(gBossCombatMusic, gBossIntroMusic, 4.5f);
//    //}
//
//    //if (gIsCrossfading) {
//    //    gFadeTimer += static_cast<f32>(Time::GetInstance().GetScaledDeltaTime());
//
//    //    // Safety check for duration to avoid division by zero
//    //    f32 t = (gFadeDuration > 0.0f) ? Clamp01(gFadeTimer / gFadeDuration) : 1.0f;
//
//    //    if (t >= 1.0f) {
//    //        AEAudioStopGroup(gFadeOutGroup);
//    //        // Set final volume to be precise
//    //        AEAudioSetGroupVolume(gFadeInGroup, 1.0f * gMusicVolume * gMasterVolume);
//    //        gIsCrossfading = false;
//    //    }
//    //    else {
//    //        // Apply curves
//    //        f32 outVol = std::cos(t * PI_2) * gMusicVolume * gMasterVolume;
//    //        f32 inVol = std::sin(t * PI_2) * gMusicVolume * gMasterVolume;
//    //        std::cout << inVol << "    " << outVol << '\n';
//    //        AEAudioSetGroupVolume(gFadeOutGroup, outVol);
//    //        AEAudioSetGroupVolume(gFadeInGroup, inVol);
//    //    }
//    //}
//}
//
//void AudioManager::LoadAll()
//{
//    if (!gAudioInitialized)
//        Init();
//
//    if (gAudioLoaded)
//        return;
//
//    // SFX
//    gPlayerAttack1 = AEAudioLoadSound("Assets/music/player_attack1.mp3");
//    gPlayerAttack2 = AEAudioLoadSound("Assets/music/player_attack2.mp3");
//    gPlayerAttack3 = AEAudioLoadSound("Assets/music/player_attack3.mp3");
//
//    // Music
//    //gMainMenuMusic = AEAudioLoadMusic("Assets/music/mainmenu.mp3");
//    gMainMenuMusic = AEAudioLoadMusic("Assets/music/MenuBGM.mp3");
//    gDeathMusic = AEAudioLoadMusic("Assets/music/Defeat.mp3");
//    gGameSceneMusic = AEAudioLoadMusic("Assets/music/GameBGM.mp3");
//    //gBossIntroMusic.audioFile = AEAudioLoadMusic("Assets/music/BossIntro.mp3");
//    //gBossCombatMusic.audioFile = AEAudioLoadMusic("Assets/music/BossFight.mp3");
//    gVictoryMusic = AEAudioLoadMusic("Assets/music/victory.mp3");
//
//    gAudioLoaded = true;
//}
//
//void AudioManager::Exit()
//{
//	// stop all audio first
//    if (AEAudioIsValidGroup(gSFXGroup))
//        AEAudioStopGroup(gSFXGroup);
//
//    if (AEAudioIsValidGroup(gMusicGroup))
//        AEAudioStopGroup(gMusicGroup);
//
//    if (AEAudioIsValidGroup(gFadeOutGroup)) 
//        AEAudioUnloadAudioGroup(gFadeOutGroup);
//    if (AEAudioIsValidGroup(gFadeInGroup)) 
//        AEAudioUnloadAudioGroup(gFadeInGroup);
//
//	// unload SFX
//    if (AEAudioIsValidAudio(gPlayerAttack1))
//        AEAudioUnloadAudio(gPlayerAttack1);
//    if (AEAudioIsValidAudio(gPlayerAttack2))
//        AEAudioUnloadAudio(gPlayerAttack2);
//    if (AEAudioIsValidAudio(gPlayerAttack3))
//        AEAudioUnloadAudio(gPlayerAttack3);
//
//    // unload Music
//    if (AEAudioIsValidAudio(gMainMenuMusic))
//        AEAudioUnloadAudio(gMainMenuMusic);
//    if (AEAudioIsValidAudio(gDeathMusic))
//        AEAudioUnloadAudio(gDeathMusic);
//    if (AEAudioIsValidAudio(gGameSceneMusic))
//        AEAudioUnloadAudio(gGameSceneMusic);
//    //if (AEAudioIsValidAudio(gBossIntroMusic.audioFile))
//    //    AEAudioUnloadAudio(gBossIntroMusic.audioFile);
//    //if (AEAudioIsValidAudio(gBossCombatMusic.audioFile))
//    //    AEAudioUnloadAudio(gBossCombatMusic.audioFile);
//    if (AEAudioIsValidAudio(gVictoryMusic))
//        AEAudioUnloadAudio(gVictoryMusic);
//
//    // unload group
//    if (AEAudioIsValidGroup(gSFXGroup))
//        AEAudioUnloadAudioGroup(gSFXGroup);
//    if (AEAudioIsValidGroup(gMusicGroup))
//        AEAudioUnloadAudioGroup(gMusicGroup);
//
//	// cleaning up state
//    gPlayerAttack1 = AEAudio{};
//    gPlayerAttack2 = AEAudio{};
//    gPlayerAttack3 = AEAudio{};
//
//    gMainMenuMusic = AEAudio{};
//    gDeathMusic = AEAudio{};
//    gGameSceneMusic = AEAudio{};
//    gBossIntroMusic.audioFile = AEAudio{};
//    gBossCombatMusic.audioFile = AEAudio{};
//    gVictoryMusic = AEAudio{};
//
//    gSFXGroup = AEAudioGroup{};
//    gMusicGroup = AEAudioGroup{};
//
//    gNextAttackIndex = 0;
//
//    gHasCurrentMusic = false;
//    gCurrentMusicId = MusicId::MainMenu;
//    gCurrentMusicVolumeScale = 1.0f;
//    gCurrentMusicPitch = 1.0f;
//    gCurrentMusicLoopCount = 0;
//
//    gAudioLoaded = false;
//    gAudioInitialized = false;
//
//    gMasterVolume = 1.0f;
//    gSFXVolume = 1.0f;
//    gMusicVolume = 1.0f;
//}
//
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

//void AudioManager::PlaySFX(SoundId id, float volumeScale, float pitch, int loopCount)
//{
//    if (!gAudioLoaded)
//        LoadAll();
//
//    AEAudio audio{};
//
//    switch (id)
//    {
//    case SoundId::PlayerAttack1:
//        audio = gPlayerAttack1;
//        break;
//    case SoundId::PlayerAttack2:
//        audio = gPlayerAttack2;
//        break;
//    case SoundId::PlayerAttack3:
//        audio = gPlayerAttack3;
//        break;
//    default:
//        return;
//    }
//
//    ApplyGroupVolumes();
//    AEAudioPlay(audio, gSFXGroup, FinalSFXVolume(volumeScale), pitch, loopCount);
//}
//
//void AudioManager::PlayMusic(MusicId id, float volumeScale, float pitch, int loopCount)
//{
//    if (!gAudioLoaded)
//        LoadAll();
//
//    AEAudio audio{};
//
//    switch (id)
//    {
//    case MusicId::MainMenu:
//        audio = gMainMenuMusic;
//        break;
//    case MusicId::Death:
//        audio = gDeathMusic;
//        break;
//    case MusicId::GameScene:
//        audio = gGameSceneMusic;
//        break;
//    case MusicId::Victory:
//        audio = gVictoryMusic;
//        break;
//    default:
//        return;
//    }
//
//    RefreshCurrentMusicState(id, volumeScale, pitch, loopCount);
//    AEAudioPlay(audio, gMusicGroup, FinalMusicVolume(volumeScale), pitch, loopCount);
//}
//
//void AudioManager::PlayNextAttackSFX()
//{
//    switch (gNextAttackIndex)
//    {
//    case 0:
//        PlaySFX(SoundId::PlayerAttack1);
//        break;
//    case 1:
//        PlaySFX(SoundId::PlayerAttack2);
//        break;
//    case 2:
//        PlaySFX(SoundId::PlayerAttack3);
//        break;
//    default:
//        break;
//    }
//
//    gNextAttackIndex = (gNextAttackIndex + 1) % 3;
//}
////void AudioManager::Crossfade(Audio& from, Audio& to, f32 duration) {
////    if (!gAudioLoaded) LoadAll();
////
////    // 1. Reset everything
////    gFadeTimer = 0.0f;
////    gFadeDuration = duration;
////    gFromChannel = &from;
////    gToChannel = &to;
////    gIsCrossfading = true;
////
////    // 2. Set INITIAL volumes immediately (t = 0)
////    // cos(0) = 1.0, sin(0) = 0.0
////    AEAudioSetGroupVolume(gFadeOutGroup, 1.0f * gMusicVolume * gMasterVolume);
////    AEAudioSetGroupVolume(gFadeInGroup, 0.0f);
////
////    // 3. Play
////    AEAudioPlay(from.audioFile, gFadeOutGroup, 1.0f, 1.0f, -1);
////    AEAudioPlay(to.audioFile, gFadeInGroup, 1.0f, 1.0f, -1);
////}