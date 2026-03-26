#include "AudioManager.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include "../Game/Time.h"
#include "../Game/Rooms/RoomData.h"
#include "../Game/Environment/traps.h"

// Declare background music.
std::unique_ptr<BGMAudio> AudioManager::bossIntroMusic = nullptr;
std::unique_ptr<BGMAudio> AudioManager::bossFightMusic = nullptr;
std::unique_ptr<BGMAudio> AudioManager::gameMusic = nullptr;
std::unique_ptr<BGMAudio> AudioManager::gameOverMusic = nullptr;
std::unique_ptr<BGMAudio> AudioManager::victoryMusic = nullptr;
std::unique_ptr<BGMAudio> AudioManager::menuMusic = nullptr;
std::unique_ptr<BGMAudio> AudioManager::creditsMusic = nullptr;

std::unique_ptr<BGMAudio> AudioManager::trapLava = nullptr;

// Declare sound effects. 
// Buff SFXs
std::unique_ptr<SFXAudio> AudioManager::buffRevealSFX = nullptr;
std::unique_ptr<SFXAudio> AudioManager::buffHoverOnceSFX = nullptr;
std::unique_ptr<SFXAudio> AudioManager::buffConfirmSFX = nullptr;

// Player SFXs
std::unique_ptr<SFXAudio> AudioManager::playerAttack1 = nullptr;
std::unique_ptr<SFXAudio> AudioManager::playerAttack2 = nullptr;
std::unique_ptr<SFXAudio> AudioManager::playerAttack3 = nullptr;
std::unique_ptr<SFXAudio> AudioManager::playerAirAttack = nullptr;
std::unique_ptr<SFXAudio> AudioManager::playerAirAttackImpact = nullptr;
std::unique_ptr<SFXAudio> AudioManager::playerJump = nullptr;
std::unique_ptr<SFXAudio> AudioManager::playerLand = nullptr;
std::unique_ptr<SFXAudio> AudioManager::playerDash = nullptr;
std::unique_ptr<SFXAudio> AudioManager::playerHurt = nullptr;
std::unique_ptr<SFXAudio> AudioManager::playerDeath = nullptr;

// Enemy SFXs
std::unique_ptr<SFXAudio> AudioManager::enemyHurt = nullptr;
std::unique_ptr<SFXAudio> AudioManager::druidCast = nullptr;
std::unique_ptr<SFXAudio> AudioManager::druidImpact = nullptr;
std::unique_ptr<SFXAudio> AudioManager::druidDeath = nullptr;
std::unique_ptr<SFXAudio> AudioManager::skeletonAttack = nullptr;
std::unique_ptr<SFXAudio> AudioManager::skeletonDeath = nullptr;

// Boss SFXs
std::unique_ptr<SFXAudio> AudioManager::bossCharging = nullptr;
std::unique_ptr<SFXAudio> AudioManager::bossProjectile = nullptr;
std::unique_ptr<SFXAudio> AudioManager::bossTeleport = nullptr;
std::unique_ptr<SFXAudio> AudioManager::bossSlash = nullptr;
std::unique_ptr<SFXAudio> AudioManager::bossDeath = nullptr;

// Trap SFXs
std::unique_ptr<SFXAudio> AudioManager::trapPressurePlate = nullptr;
std::unique_ptr<SFXAudio> AudioManager::trapSpikes = nullptr;

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

    // Music flags to ensure they play only once.
    bool gIsPlayingBoss2ndPhase = false;
    bool gIsPlayingGOver = false;
    bool gIsPlayingVictory = false;

    BGMAudio* gCurrTrack = nullptr;

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
    baseVolume = initialGroupVol;
    ApplyFinalVolume();
    AEAudioPlay(audioFile, ownGroup, 1.0f, pitch, -1);
    active = true;
    gCurrTrack = this;
}
void BGMAudio::Stop() {
    if (AEAudioIsValidGroup(ownGroup))
        AEAudioStopGroup(ownGroup);
    active = false;
}
void BGMAudio::SetVolume(f32 const& vol) {
    baseVolume = vol;
    ApplyFinalVolume();
}
void BGMAudio::ApplyFinalVolume() {
    float finalVol = baseVolume * fadeVolume * gMusicVolume * gMasterVolume;

    if (AEAudioIsValidGroup(ownGroup))
        AEAudioSetGroupVolume(ownGroup, finalVol);
}
void BGMAudio::CrossfadeTo(BGMAudio& other, f32 duration) {
    // Stop any existing crossfade to prevent timer glitches
    gIsCrossfading = false;

    gFadeOutTrack = this;
    gFadeInTrack = &other;

    if (!other.IsActive()) {
        other.Play(0.0f);  // start silent
        other.SetActive(true);
    }

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

    // Default first track to vocal
    if (!bossIntroMusic->IsActive()) {
        bossIntroMusic->Play(bossIntroMusic->GetVolume());
        bossIntroMusic->SetActive(true);
        bossFightMusic->Play(0.0f);
        bossFightMusic->SetActive(true);
        gCurrTrack = bossIntroMusic.get();
    }
    if (roomMgr.GetCurrentRoomID() == ROOM_11) {
        // Triggered boss phase 2
        if (boss.phase2 && !gIsPlayingBoss2ndPhase) { // To remove check triggered when rooms are spawned properly.
            //std::cout << "2ND PHASE" << '\n';
            bossIntroMusic->CrossfadeTo(*bossFightMusic, 1.2f);
            gIsPlayingBoss2ndPhase = true;
            gCurrTrack = bossFightMusic.get();
        }
        if (boss.IsDead()) {
            if (!gIsPlayingVictory) {
                if (gCurrTrack) {
                    gCurrTrack->CrossfadeTo(*victoryMusic, 1.2f);
                    gCurrTrack = victoryMusic.get();
                }
                victoryMusic->Play(victoryMusic->GetVolume());
            }
            gIsPlayingVictory = true;
        }
    }
}
void AudioManager::PlayGameOverMusic() {
    if (!gIsPlayingGOver) {
        if (gCurrTrack) {
            gCurrTrack->CrossfadeTo(*gameOverMusic, 1.2f);
        }
        else {
            gameOverMusic->Play(gameOverMusic->GetVolume()); // fallback
        }
        gCurrTrack = gameOverMusic.get();
        gIsPlayingGOver = true;
    }
}
void AudioManager::PlayMenuMusic() {
    if (gCurrTrack && gCurrTrack != menuMusic.get())
    {
        gCurrTrack->CrossfadeTo(*menuMusic, 1.2f);
    }
    else if (!menuMusic->IsActive())
    {
        menuMusic->Play(menuMusic->GetVolume());
    }
    gCurrTrack = menuMusic.get();
}
void AudioManager::PlayGameMusic() {
    if (gCurrTrack && gCurrTrack != gameMusic.get())
    {
        gCurrTrack->CrossfadeTo(*gameMusic, 1.2f);
    }
    else if (!gameMusic->IsActive())
    {
        gameMusic->Play(gameMusic->GetVolume());
    }
    if (AudioManager::trapLava && !AudioManager::trapLava->IsActive())
    {
        AudioManager::trapLava->Play(0.0f); // start silent
        AudioManager::trapLava->SetActive(true);
    }
    gCurrTrack = gameMusic.get();
}
void AudioManager::MuffleMusic() {
    if (!gIsMuffled) {
        preservedGameVol = gCurrTrack->GetVolume();
        gIsMuffled = true;
    }
    gCurrTrack->SetVolume(preservedGameVol * 0.55f);
    gCurrTrack->ApplyFinalVolume();
    RefreshAllMusicVolumes();
}
void AudioManager::UnmuffleMusic() {
    gCurrTrack->SetVolume(preservedGameVol);
    gIsMuffled = false;
    RefreshAllMusicVolumes();
}
void AudioManager::RefreshAllMusicVolumes()
{
    if (bossIntroMusic && bossIntroMusic->IsActive())
        bossIntroMusic->ApplyFinalVolume();

    if (bossFightMusic && bossFightMusic->IsActive())
        bossFightMusic->ApplyFinalVolume();

    if (gameMusic && gameMusic->IsActive())
        gameMusic->ApplyFinalVolume();

    if (gameOverMusic && gameOverMusic->IsActive()) {
        gameOverMusic->ApplyFinalVolume();
    }
    if (victoryMusic && victoryMusic->IsActive()) {
        victoryMusic->ApplyFinalVolume();
    }
    if (menuMusic && menuMusic->IsActive()) {
        menuMusic->ApplyFinalVolume();
    }
    if (creditsMusic && creditsMusic->IsActive()) {
        creditsMusic->ApplyFinalVolume();
    }
    if (trapLava && trapLava->IsActive()) {
        trapLava->ApplyFinalVolume();
    }
}
void AudioManager::PlaySFX(SFXAudio const& sfx, f32 const& volume, f32 const& pitch) {
    const AEAudio& audio = sfx.GetAudio();
    if (AEAudioIsValidAudio(audio))
    {
        AEAudioPlay(audio, gSFXGroup, volume, pitch, 0);
    }
}
void AudioManager::UpdateLavaAudio(const TrapManager& trapMgr, const Player& player)
{
    if (!trapLava || !trapLava->IsActive()) {
        return;
    }

    float dist = trapMgr.GetClosestLavaDistance(player.GetPosition());

    if (dist < 0.0f)
    {
        trapLava->SetVolume(0.0f);
        return;
    }

    float maxDistance = 10.0f;

    float volume = 1.0f - (dist / maxDistance);
    volume = Clamp01(volume);

    // optional: don't go fully silent
    volume = 0.1f + 0.9f * volume;

    trapLava->SetVolume(volume);
    trapLava->ApplyFinalVolume();
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
    if (!gameOverMusic)
        gameOverMusic = std::make_unique<BGMAudio>("Assets/music/Defeat.mp3");
    if (!victoryMusic)
        victoryMusic = std::make_unique<BGMAudio>("Assets/music/Victory.mp3");
    if (!menuMusic)
        menuMusic = std::make_unique<BGMAudio>("Assets/music/MenuBGM.mp3");
    if (!creditsMusic)
        creditsMusic = std::make_unique<BGMAudio>("Assets/music/Credits.mp3");
    if (!trapLava)
        trapLava = std::make_unique<BGMAudio>("Assets/music/TrapLava.mp3");

    // Load sound effects here.
    // Buff sfxs
    if (!buffRevealSFX)
        buffRevealSFX = std::make_unique<SFXAudio>("Assets/music/BuffRevealSFX.mp3");
    if (!buffHoverOnceSFX)
        buffHoverOnceSFX = std::make_unique<SFXAudio>("Assets/music/BuffHoverOnceSFX.mp3");
    if (!buffConfirmSFX)
        buffConfirmSFX = std::make_unique<SFXAudio>("Assets/music/BuffConfirmSFX.mp3");

    // Player sfxs
    if (!playerAttack1)
        playerAttack1 = std::make_unique<SFXAudio>("Assets/music/PlayerAttack1.mp3");
    if (!playerAttack2)
        playerAttack2 = std::make_unique<SFXAudio>("Assets/music/PlayerAttack2.mp3");
    if (!playerAttack3)
        playerAttack3 = std::make_unique<SFXAudio>("Assets/music/PlayerAttack3.mp3");
    if (!playerAirAttack)
        playerAirAttack = std::make_unique<SFXAudio>("Assets/music/PlayerAirAttack.mp3");
    if (!playerAirAttackImpact)
        playerAirAttackImpact = std::make_unique<SFXAudio>("Assets/music/PlayerAirAttackImpact.mp3");
    if (!playerJump)
        playerJump = std::make_unique<SFXAudio>("Assets/music/PlayerJump.mp3");
    if (!playerLand)
        playerLand = std::make_unique<SFXAudio>("Assets/music/PlayerLand.mp3");
    if (!playerDash)
        playerDash = std::make_unique<SFXAudio>("Assets/music/PlayerDash.mp3");
    if (!playerHurt)
        playerHurt = std::make_unique<SFXAudio>("Assets/music/PlayerHurt.mp3");
    if (!playerDeath)
        playerDeath = std::make_unique<SFXAudio>("Assets/music/PlayerDeath.mp3");

    // Enemy SFXs
    if (!enemyHurt)
        enemyHurt = std::make_unique<SFXAudio>("Assets/music/EnemyHurt.mp3");
    if (!druidCast)
        druidCast = std::make_unique<SFXAudio>("Assets/music/DruidCast.mp3");
    if (!druidImpact)
        druidImpact = std::make_unique<SFXAudio>("Assets/music/DruidImpact.mp3");
    if (!druidDeath)
        druidDeath = std::make_unique<SFXAudio>("Assets/music/DruidDeath.mp3");
    if (!skeletonAttack)
        skeletonAttack = std::make_unique<SFXAudio>("Assets/music/SkeletonAttack.mp3");
    if (!skeletonDeath)
        skeletonDeath = std::make_unique<SFXAudio>("Assets/music/SkeletonDeath.mp3");

    // Boss SFXs
    if (!bossCharging)
        bossCharging = std::make_unique<SFXAudio>("Assets/music/BossCharging.mp3");
    if (!bossProjectile)
        bossProjectile = std::make_unique<SFXAudio>("Assets/music/BossProjectile.mp3");
    if (!bossTeleport)
        bossTeleport = std::make_unique<SFXAudio>("Assets/music/BossTeleport.mp3");
    if (!bossSlash)
        bossSlash = std::make_unique<SFXAudio>("Assets/music/BossSlash.mp3");
    if (!bossDeath)
        bossDeath = std::make_unique<SFXAudio>("Assets/music/BossDeath.mp3");
    if (!trapPressurePlate)
        trapPressurePlate = std::make_unique<SFXAudio>("Assets/music/TrapPressurePlate.mp3");
    if (!trapSpikes)
        trapSpikes = std::make_unique<SFXAudio>("Assets/music/TrapSpikes.mp3");

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
    StopAllMusic();
    ResetRuntimeState();

    // Reset background music.
    bossIntroMusic.reset();
    bossFightMusic.reset();
    gameMusic.reset();
    gameOverMusic.reset();
    victoryMusic.reset();
    menuMusic.reset();
    creditsMusic.reset();

    // Reset lava ambient sound.
    trapLava.reset();

    // Reset buff card sfxs.
    buffRevealSFX.reset();
    buffHoverOnceSFX.reset();
    buffConfirmSFX.reset();

    // Reset player sfxs.
    playerAttack1.reset();
    playerAttack2.reset();
    playerAttack3.reset();
    playerAirAttack.reset();
    playerAirAttackImpact.reset();
    playerJump.reset();
    playerLand.reset();
    playerDash.reset();
    playerHurt.reset();
    playerDeath.reset();

    // Reset enemy sfxs.
    enemyHurt.reset();
    druidCast.reset();
    druidImpact.reset();
    druidDeath.reset();
    skeletonAttack.reset();
    skeletonDeath.reset();

    // Reset boss sfxs.
    bossCharging.reset();
    bossProjectile.reset();
    bossTeleport.reset();
    bossSlash.reset();
    bossDeath.reset();

    // Reset trap sfxs.
    trapSpikes.reset();
    trapPressurePlate.reset();

    // Reset music groups.
    AEAudioUnloadAudioGroup(gMusicGroup);
    AEAudioUnloadAudioGroup(gSFXGroup);

    // Reset audio groups to default state to prevent dangling pointers on reinit
    gMusicGroup = {};
    gSFXGroup = {};
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

void AudioManager::StopAllMusic()
{
    if (gameMusic) gameMusic->Stop();
    if (bossIntroMusic) bossIntroMusic->Stop();
    if (bossFightMusic) bossFightMusic->Stop();
    if (gameOverMusic) gameOverMusic->Stop();
    if (victoryMusic) victoryMusic->Stop();
}

void AudioManager::ResetRuntimeState()
{
    gIsMuffled = false;

    gFadeTimer = 0.0f;
    gFadeDuration = 0.0f;
    preservedGameVol = 1.0f;

    gIsCrossfading = false;
    gFadeOutTrack = nullptr;
    gFadeInTrack = nullptr;
    gIsPlayingBoss2ndPhase = false;
    gIsPlayingGOver = false;
    gIsPlayingVictory = false;

    gCurrTrack = nullptr;

    AudioManager::playedBossDeathSFX = false;
    AudioManager::playedBossChargingSFX = false;
    AudioManager::playedBossTeleportSFX = false;
}

void AudioManager::ResetForRestart()
{
    StopAllMusic();
    ResetRuntimeState();
    gCurrTrack = nullptr;
}
