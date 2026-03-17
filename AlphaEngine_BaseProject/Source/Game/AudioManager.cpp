#include "AudioManager.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include "../Game/Time.h"

// Declare background music.
std::unique_ptr<BGMAudio> AudioManager::bossIntroMusic = nullptr;
std::unique_ptr<BGMAudio> AudioManager::bossFightMusic = nullptr;
std::unique_ptr<BGMAudio> AudioManager::gameMusic = nullptr;
std::unique_ptr<BGMAudio> AudioManager::bossInstrMusic = nullptr;

// Declare sound effects.
//std::unique_ptr<SFXAudio> AudioManager::buffFlipSFX = nullptr;
std::unique_ptr<SFXAudio> AudioManager::buffRevealSFX = nullptr;
std::unique_ptr<SFXAudio> AudioManager::buffHoverOnceSFX = nullptr;
std::unique_ptr<SFXAudio> AudioManager::buffConfirmSFX = nullptr;
namespace
{
    const f32 PI_2 = 1.57079632679f;
    bool gIsMuffled = false;

    float gMasterVolume = 1.0f;
    float gSFXVolume = 1.0f;
    float gMusicVolume = 1.0f;

    AEAudioGroup gSFXGroup{};
    AEAudioGroup gMusicGroup{};

    // Crossfade variables
    f32 gFadeTimer = 0.0f;
    f32 gFadeDuration = 0.0f;
    f32 preservedGameVol{};
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
/*--------------------------------------------------
|                                                  |
|                   Background Music               |
|                                                  |
--------------------------------------------------*/
BGMAudio::BGMAudio(char const* filename)  // Ctor for BGMAudio. Default ctor is disallowed.
    : baseVolume{ 1.0f }, fadeVolume{ 1.0f }, pitch{ 1.0f }, active{ false } // Initialize variables
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
void BGMAudio::Play(f32 const& initialGroupVol) {
    // 1. Set the internal volume state (the group multiplier)
    baseVolume = initialGroupVol;

    // 2. Set the Group Volume immediately
    ApplyFinalVolume();

    // 3. CRITICAL: Set the instance volume to 1.0f 
    // This way the sound is "full strength" but muted by its parent group
    AEAudioPlay(audioFile, ownGroup, 1.0f, pitch, -1);
}
void BGMAudio::Stop() {
    if (AEAudioIsValidGroup(ownGroup))
        AEAudioStopGroup(ownGroup);
}
void BGMAudio::SetVolume(f32 const& vol) {
    baseVolume = vol;
    ApplyFinalVolume();
}
void BGMAudio::ApplyFinalVolume() {
    float finalVol = baseVolume * fadeVolume * gMusicVolume * gMasterVolume;

    if (AEAudioIsValidGroup(ownGroup))
        AEAudioSetGroupVolume(ownGroup, finalVol);
    AudioManager::RefreshAllMusicVolumes();
}
void BGMAudio::CrossfadeTo(BGMAudio& other, f32 duration) {
    // Stop any existing crossfade to prevent timer glitches
    gIsCrossfading = false;

    gFadeOutTrack = this;
    gFadeInTrack = &other;
    gFadeTimer = 0.0f;
    gFadeDuration = duration;
    gIsCrossfading = true;

}
/*--------------------------------------------------
|                                                  |
|                   Sound Effects                  |
|                                                  |
--------------------------------------------------*/
SFXAudio::SFXAudio(char const* filename) {
    audioFile = AEAudioLoadSound(filename); // Load sfx file
    if (!AEAudioIsValidAudio(audioFile)) {
        std::cout << "Failed to load SFX: " << filename << "\n";
    }
}
SFXAudio::~SFXAudio() { // Dtor for automatic cleanup
    if (AEAudioIsValidAudio(audioFile)) {
        AEAudioUnloadAudio(audioFile);
        audioFile = {};
    }
}
void AudioManager::PlayBossMusic(EnemyBoss const& boss, RoomManager const& roomMgr) {
    // For the first time, play both tracks together so they sync perfectly and align
    // with crossfade effect.
    if (!bossIntroMusic->IsActive()) {
        bossIntroMusic->Play(bossIntroMusic->GetVolume());
        bossIntroMusic->SetActive(true);
        bossFightMusic->Play(0.0f);
        bossFightMusic->SetActive(true);
    }
    // Triggered aggro
    if (boss.bossEngaged || AEInputCheckTriggered(AEVK_2)) { // To remove check triggered when rooms are spawned properly.
       bossIntroMusic->CrossfadeTo(*bossFightMusic, 1.2f);
    }
    // Lose aggro but still in boss room
    else if (!boss.bossEngaged && (roomMgr.GetCurrentRoomID() == ROOM_10) || AEInputCheckTriggered(AEVK_1)) { // To remove check triggered when rooms are spawned properly.
        bossFightMusic->CrossfadeTo(*bossIntroMusic, 1.2f);
    }
}
void AudioManager::MuffleGameMusic() {
    if (!gIsMuffled) {
        preservedGameVol = gameMusic->GetVolume();
        gIsMuffled = true;
    }
    gameMusic->SetVolume(preservedGameVol * 0.55f);
    gameMusic->ApplyFinalVolume();
    RefreshAllMusicVolumes();
}
void AudioManager::UnmuffleGameMusic() {
    gameMusic->SetVolume(preservedGameVol);
    gIsMuffled = false;
    RefreshAllMusicVolumes();
}
void AudioManager::RefreshAllMusicVolumes()
{
    if (bossIntroMusic && bossIntroMusic->IsActive())
        bossIntroMusic->ApplyFinalVolume();

    if (bossFightMusic && bossFightMusic->IsActive())
        bossFightMusic->ApplyFinalVolume();

    if (bossInstrMusic && bossInstrMusic->IsActive())
        bossInstrMusic->ApplyFinalVolume();

    if (gameMusic && gameMusic->IsActive())
        gameMusic->ApplyFinalVolume();
}
void AudioManager::PlaySFX(SFXAudio const& sfx, f32 const& pitch) {
    const AEAudio& audio = sfx.GetAudio();
    if (AEAudioIsValidAudio(audio))
    {
        AEAudioPlay(audio, gSFXGroup, 1.0f, pitch, 0);
    }
}
/*--------------------------------------------------
|                                                  |
|                   AudioMgr Init                  |
|                                                  |
--------------------------------------------------*/
void AudioManager::Init() {
    if (!AEAudioIsValidGroup(gMusicGroup)) {
        gMusicGroup = AEAudioCreateGroup();
    }
    if (!AEAudioIsValidGroup(gSFXGroup)) {
        gSFXGroup = AEAudioCreateGroup();
    }
    ApplyGroupVolumes(); // Ensure the group volume is applied

    // Load BGM files here.
    if (!bossIntroMusic)
        bossIntroMusic = std::make_unique<BGMAudio>("Assets/music/BossIntro.mp3");
    if (!bossFightMusic)
        bossFightMusic = std::make_unique<BGMAudio>("Assets/music/BossFight.mp3");
    if (!gameMusic)
        gameMusic = std::make_unique<BGMAudio>("Assets/music/GameBGM.mp3");
    if (!bossInstrMusic)
        bossInstrMusic = std::make_unique<BGMAudio>("Assets/music/BossInstrumental.mp3");

    // Load sound effects here.
    //if (!buffFlipSFX)
    //    buffFlipSFX = std::make_unique<SFXAudio>("Assets/music/BuffFlipSFX.mp3");
    if (!buffRevealSFX)
        buffRevealSFX = std::make_unique<SFXAudio>("Assets/music/BuffRevealSFX.mp3");
    if (!buffHoverOnceSFX)
        buffHoverOnceSFX = std::make_unique<SFXAudio>("Assets/music/BuffHoverOnceSFX.mp3");
    if (!buffConfirmSFX)
        buffConfirmSFX = std::make_unique<SFXAudio>("Assets/music/BuffConfirmSFX.mp3");

    //if (!menuMusic)
    //    menuMusic = std::make_unique<BGMAudio>("Assets/music/MenuBGM.mp3");
    //bossIntroMusic->Play(bossIntroMusic->GetVolume());
    //bossIntroMusic->SetActive(true);
    //bossFightMusic->Play(0.0f);
    //bossFightMusic->SetActive(true);
}

void AudioManager::Update() {
    // Update crossfade variables for volumes of fadein and fadeout track.
    if (gIsCrossfading && gFadeOutTrack && gFadeInTrack) {
        gFadeTimer += static_cast<f32>(Time::GetInstance().GetDeltaTime());
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
            gFadeOutTrack->ApplyFinalVolume();
            gFadeInTrack->ApplyFinalVolume();
        }
    }
}
void AudioManager::Exit() {
    // Call BGMAudio dtor by reset.
    bossIntroMusic.reset();
    bossFightMusic.reset();
    bossInstrMusic.reset();
    gameMusic.reset();

    // Call SFXAudio dtor by reset.
    //buffFlipSFX.reset();
    buffRevealSFX.reset();
    buffHoverOnceSFX.reset();
    buffConfirmSFX.reset();

    // Unload both music groups.
    AEAudioUnloadAudioGroup(gMusicGroup);
    AEAudioUnloadAudioGroup(gSFXGroup);
}
void AudioManager::SetMasterVolume(float v)
{
    gMasterVolume = Clamp01(v);
    ApplyGroupVolumes();
    RefreshAllMusicVolumes();
}
void AudioManager::SetMusicVolume(float v)
{
    gMusicVolume = Clamp01(v);
    ApplyGroupVolumes();
    RefreshAllMusicVolumes();
}
void AudioManager::SetSFXVolume(float v)
{
    gSFXVolume = Clamp01(v);
    ApplyGroupVolumes();
    RefreshAllMusicVolumes();
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
