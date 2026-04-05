/*!
@file		AudioManager.cpp
@author 	Wei Xiang NG, Jiang Chengyan (base system)
@brief		This C++ file implements the AudioManager class for managing all audio in the game, 
            including background music and sound effects. It handles loading audio files, playing 
            tracks based on game events (e.g. boss fights, menu navigation), adjusting 
            volumes, crossfading between tracks, and updating audio states such as muffling 
            during certain gameplay scenarios. The AudioManager ensures a cohesive audio 
			experience that responds dynamically to the player's actions and game state.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
#include "AudioManager.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include "../Game/Environment/traps.h"
#include "../Game/Rooms/RoomData.h"
#include "../Game/Time.h"

/*-----------------------------------------------------------------------------
                 Static member definitions : Background Music
-----------------------------------------------------------------------------*/
std::unique_ptr<BGMAudio> AudioManager::bossIntroMusic = nullptr;
std::unique_ptr<BGMAudio> AudioManager::bossFightMusic = nullptr;
std::unique_ptr<BGMAudio> AudioManager::gameMusic = nullptr;
std::unique_ptr<BGMAudio> AudioManager::gameOverMusic = nullptr;
std::unique_ptr<BGMAudio> AudioManager::victoryMusic = nullptr;
std::unique_ptr<BGMAudio> AudioManager::menuMusic = nullptr;
std::unique_ptr<BGMAudio> AudioManager::creditsMusic = nullptr;
std::unique_ptr<BGMAudio> AudioManager::trapLava = nullptr;

/*-----------------------------------------------------------------------------
                  Static member definitions : Sound Effects
-----------------------------------------------------------------------------*/

// Buff SFXs
std::unique_ptr<SFXAudio> AudioManager::buffRevealSFX = nullptr;
std::unique_ptr<SFXAudio> AudioManager::buffHoverOnceSFX = nullptr;
std::unique_ptr<SFXAudio> AudioManager::buffConfirmSFX = nullptr;

// UI SFXs
std::unique_ptr<SFXAudio> AudioManager::buttonClick = nullptr;
std::unique_ptr<SFXAudio> AudioManager::healthPickup = nullptr;

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
std::unique_ptr<SFXAudio> AudioManager::enemyHurtCrit = nullptr;
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

/*-----------------------------------------------------------------------------
Internal helper functions and variables for the AudioManager class. 

These include constants for calculations, state variables for tracking the 
current audio state, and utility functions for clamping volume values and 
applying group volumes.
-----------------------------------------------------------------------------*/
namespace
{
    const f32 PI_2 = 1.57079632679f;

    bool gIsMuffled = false;

    float gMasterVolume = 1.0f;
    float gSFXVolume = 1.0f;
    float gMusicVolume = 1.0f;

    AEAudioGroup gSFXGroup{};
    AEAudioGroup gMusicGroup{};

    // Crossfade state
    f32  gFadeTimer = 0.0f;
    f32  gFadeDuration = 0.0f;
    f32  preservedGameVol{};
    bool gIsCrossfading = false;

    BGMAudio* gFadeOutTrack = nullptr;
    BGMAudio* gFadeInTrack = nullptr;

    // Music flags to ensure they play only once
    bool gIsPlayingBoss2ndPhase = false;
    bool gIsPlayingGOver = false;
    bool gIsPlayingVictory = false;

    BGMAudio* gCurrTrack = nullptr;
    /*-----------------------------------------------------------------------------
	This function clamps a float value between 0.0 and 1.0, ensuring that volume 
    levels do not exceed the valid range for audio settings.
    -----------------------------------------------------------------------------*/
    float Clamp01(float v)
    {
        return (std::max)(0.0f, (std::min)(1.0f, v));
    }
    /*-----------------------------------------------------------------------------
	This function calculates the final volume for sound effects by applying the 
	master volume and the SFX volume settings, ensuring that the resulting volume 
	is within the valid range of 0.0 to 1.0.
    -----------------------------------------------------------------------------*/
    float FinalSFXVolume(float scale)
    {
        return Clamp01(scale);
    }
    /*-----------------------------------------------------------------------------
	This function calculates the final volume for music by applying the master 
	volume and the music volume settings, ensuring that the resulting volume is 
	within the valid range of 0.0 to 1.0. 

	This allows for consistent volume control across all music tracks in the game, 
    while still respecting the individual track's base volume and any fade effects.
    -----------------------------------------------------------------------------*/
    float FinalMusicVolume(float scale)
    {
        return Clamp01(scale);
    }
    /*-----------------------------------------------------------------------------
	This function applies the current master, SFX, and music volume settings to the
	respective audio groups. It calculates the effective volume for each group by
	multiplying the master volume with the individual group volume (SFX or music)
	and then sets the group volume using the AEAudioSetGroupVolume function. 
    
	This ensures that any changes to the master volume or individual group volumes 
	are reflected in the actual audio output of the game, allowing for dynamic 
	volume adjustments based on player preferences or game events.
    -----------------------------------------------------------------------------*/
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


/*-----------------------------------------------------------------------------
Background Music (BGM) management for the AudioManager class. 

This section includes the implementation of the BGMAudio class, which handles 
loading, playing, stopping, and crossfading background music tracks. 

Each BGMAudio instance manages its own audio group for independent control 
over volume and playback, allowing for features like crossfading between tracks 
and dynamic volume adjustments based on game events.

                                BACKGROUND MUSIC
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
This constructor for the BGMAudio class loads a music file and creates an 
audio group for the instance. It takes a filename as a parameter, which is 
used to load the music using the AEAudioLoadMusic function. 

The constructor also checks if the audio was loaded successfully and 
prints an error message if it fails. The audio group is created using the 
AEAudioCreateGroup function, which allows for individual control over the 
volume and playback of this specific track, enabling features like crossfading 
and independent volume adjustments.
-----------------------------------------------------------------------------*/
BGMAudio::BGMAudio(char const* filename)
    : baseVolume{ 1.0f },
    fadeVolume{ 1.0f },
    pitch{ 1.0f },
    active{ false }
{
    audioFile = AEAudioLoadMusic(filename);
    ownGroup = AEAudioCreateGroup();

    if (!AEAudioIsValidAudio(audioFile))
    {
        std::cout << "Failed to load audio: " << filename << "\n";
    }
}
/*-----------------------------------------------------------------------------
This destructor for the BGMAudio class ensures that any loaded audio and 
created audio groups are properly unloaded and cleaned up when an instance 
of BGMAudio is destroyed.
-----------------------------------------------------------------------------*/
BGMAudio::~BGMAudio()
{
    if (AEAudioIsValidGroup(ownGroup))
    {
        AEAudioStopGroup(ownGroup);
        AEAudioUnloadAudioGroup(ownGroup);
        ownGroup = {};
    }

    if (AEAudioIsValidAudio(audioFile))
    {
        AEAudioUnloadAudio(audioFile);
        audioFile = {};
    }
}
/*-----------------------------------------------------------------------------
This function plays the background music track associated with this BGMAudio 
instance. It takes an initial group volume as a parameter, which is used to 
set the base volume for the track before applying the final volume calculations.

The function calls AEAudioPlay to start playing the audio file on the 
instance's audio group, and it sets the active flag to true to indicate that 
the track is currently playing. 

It also updates the global current track pointer to this instance, allowing the 
AudioManager to keep track of which music track is currently active for features 
like crossfading and volume adjustments.
-----------------------------------------------------------------------------*/
void BGMAudio::Play(f32 const& initialGroupVol)
{
    baseVolume = initialGroupVol;
    ApplyFinalVolume();
    AEAudioPlay(audioFile, ownGroup, 1.0f, pitch, -1);
    active = true;
    gCurrTrack = this;
}
/*-----------------------------------------------------------------------------
This function stops the background music track associated with this 
BGMAudio instance.
-----------------------------------------------------------------------------*/
void BGMAudio::Stop()
{
    if (AEAudioIsValidGroup(ownGroup))
        AEAudioStopGroup(ownGroup);

    active = false;
    fadeVolume = 1.0f;
    baseVolume = 1.0f;
}
/*-----------------------------------------------------------------------------
This function sets the base volume for the BGMAudio instance and then applies 
the final volume calculations to update the actual output volume of the track. 

The base volume is a multiplier that can be adjusted for individual tracks, 
and when ApplyFinalVolume is called, it combines the base volume with any fade 
effects and the global music and master volume settings to determine the final 
volume that is sent to the audio system. 

This allows for dynamic volume control that can respond to game events 
(like fading out during a transition) while still respecting the player's 
overall volume preferences.
-----------------------------------------------------------------------------*/
void BGMAudio::SetVolume(f32 const& vol)
{
    baseVolume = vol;
    ApplyFinalVolume();
}
/*-----------------------------------------------------------------------------
This function calculates and applies the final volume for the BGMAudio 
instance by combining the base volume, any fade effects, and the global 
music and master volume settings. 

It multiplies these factors together to determine the final volume level for 
the track and then sets the group volume using AEAudioSetGroupVolume.
-----------------------------------------------------------------------------*/
void BGMAudio::ApplyFinalVolume()
{
    float finalVol = baseVolume * fadeVolume * gMusicVolume * gMasterVolume;

    if (AEAudioIsValidGroup(ownGroup))
        AEAudioSetGroupVolume(ownGroup, finalVol);
}
/*-----------------------------------------------------------------------------
This function initiates a crossfade transition from the current BGMAudio track 
to another specified BGMAudio track over a given duration. 

It sets up the necessary state for the crossfade, including which track is 
fading out and which is fading in, and it starts the fade timer. 

If the target track is not already active, it will start playing it at zero 
volume so that it can fade in smoothly. The function also ensures that any 
existing crossfade is stopped to prevent glitches in the timing of the 
fade effect.
-----------------------------------------------------------------------------*/
void BGMAudio::CrossfadeTo(BGMAudio& other, f32 duration)
{
    // Stop any existing crossfade to prevent timer glitches
    gIsCrossfading = false;

    gFadeOutTrack = this;
    gFadeInTrack = &other;

    if (!other.IsActive())
    {
        other.Play(0.0f); // start silent
        other.SetActive(true);
    }

    gFadeTimer = 0.0f;
    gFadeDuration = duration;
    gIsCrossfading = true;
}

/*-----------------------------------------------------------------------------
Sound Effects (SFX) management for the AudioManager class. 

This section includes the implementation of the SFXAudio class, which handles 
loading and managing sound effects. Each SFXAudio instance loads a sound effect 
file and provides access to the AEAudio object for playback. 

The AudioManager uses these SFXAudio instances to play sound effects in 
response to game events, applying the appropriate volume settings based on the 
global SFX volume and master volume.

                                SOUND EFFECTS
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
This constructor for the SFXAudio class loads a sound effect file using the
AEAudioLoadSound function. 

It takes a filename as a parameter and attempts to load the sound effect, 
storing the resulting AEAudio object in the audioFile member variable.
-----------------------------------------------------------------------------*/
SFXAudio::SFXAudio(char const* filename)
{
    audioFile = AEAudioLoadSound(filename);

    if (!AEAudioIsValidAudio(audioFile))
    {
        std::cout << "Failed to load SFX: " << filename << "\n";
    }
}
/*-----------------------------------------------------------------------------
This destructor for the SFXAudio class ensures that any loaded sound effect 
audio is properly unloaded when an instance of SFXAudio is destroyed.
-----------------------------------------------------------------------------*/
SFXAudio::~SFXAudio()
{
    if (AEAudioIsValidAudio(audioFile))
    {
        AEAudioUnloadAudio(audioFile);
        audioFile = {};
    }
}
/*-----------------------------------------------------------------------------
Background Music Control for the AudioManager class.

This section includes functions for playing different background music tracks 
based on game events (e.g. boss fights, menu navigation), as well as functions 
for muffling and unmuffling music during certain gameplay scenarios. 

The AudioManager manages the current active track and handles crossfading 
between tracks to ensure smooth transitions in the game's audio experience. 

It also includes a function to refresh the volumes of all music tracks, 
which is useful when global volume settings are changed or when applying 
effects like muffling.

                          BACKGROUND MUSIC CONTROL
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
This function manages the background music during a boss fight, handling the 
transition from the boss intro music to the boss fight music, and eventually to 
the victory music when the boss is defeated.
-----------------------------------------------------------------------------*/
void AudioManager::PlayBossMusic(EnemyBoss const& boss, RoomManager const& roomMgr)
{
    // For the first time, play both tracks together so they sync perfectly
    // and align with the crossfade effect.

    if (!playedBossIntroMusic)
    {
        bossIntroMusic->Play(bossIntroMusic->GetVolume());
        bossFightMusic->Play(0.0f);
        gCurrTrack = bossIntroMusic.get();
        playedBossIntroMusic = true;
    }

    if (roomMgr.GetCurrentRoomID() == ROOM_11)
    {
        if (boss.phase2 && !gIsPlayingBoss2ndPhase)
        {
            bossIntroMusic->CrossfadeTo(*bossFightMusic, 1.2f);
            gIsPlayingBoss2ndPhase = true;
            gCurrTrack = bossFightMusic.get();
        }

        if (boss.IsDead() && !gIsPlayingVictory)
        {
            gIsPlayingVictory = true;

            if (gCurrTrack)
            {
                gCurrTrack->CrossfadeTo(*victoryMusic, 1.2f);
                gCurrTrack = victoryMusic.get();
            }
            else
            {
                victoryMusic->Play(victoryMusic->GetVolume());
                gCurrTrack = victoryMusic.get();
            }

            if (bossIntroMusic && bossIntroMusic.get() != gCurrTrack)
                bossIntroMusic->SetVolume(0.0f);

            if (bossFightMusic && bossFightMusic.get() != gCurrTrack)
                bossFightMusic->SetVolume(0.0f);

            if (trapLava && trapLava->IsActive())
                trapLava->Stop();
        }
    }
}
/*-----------------------------------------------------------------------------
This function manages the transition to the game over music when the player is 
defeated.
-----------------------------------------------------------------------------*/
void AudioManager::PlayGameOverMusic()
{
    if (!gIsPlayingGOver)
    {
        if (gCurrTrack)
        {
            gCurrTrack->CrossfadeTo(*gameOverMusic, 1.2f);
        }
        else
        {
            gameOverMusic->Play(gameOverMusic->GetVolume());
        }

        gCurrTrack = gameOverMusic.get();
        gIsPlayingGOver = true;
    }
}
/*-----------------------------------------------------------------------------
This function manages the transition to the menu music when the player is in 
the main menu or navigating menu screens. 

It handles crossfading from the current track to the menu music if a 
different track is currently active, or simply plays the menu music if it is 
not already active.
-----------------------------------------------------------------------------*/
void AudioManager::PlayMenuMusic()
{
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
/*-----------------------------------------------------------------------------
This function manages the transition to the game music during regular gameplay.
-----------------------------------------------------------------------------*/
void AudioManager::PlayGameMusic()
{
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
        AudioManager::trapLava->Play(0.0f);
        AudioManager::trapLava->SetActive(true);
    }

    gCurrTrack = gameMusic.get();
}
/*-----------------------------------------------------------------------------
This function manages the transition to the credits music when the player 
reaches the credits screen. 

It handles crossfading from the current track to the credits music if a 
different track is currently active, or simply plays the credits music if it 
is not already active.
-----------------------------------------------------------------------------*/
void AudioManager::PlayCreditsMusic() {
    if (gCurrTrack && gCurrTrack != creditsMusic.get())
    {
        gCurrTrack->CrossfadeTo(*creditsMusic, 1.2f);
    }
    else if (!creditsMusic->IsActive())
    {
        creditsMusic->Play(creditsMusic->GetVolume());
    }
	gCurrTrack = creditsMusic.get();
}
/*-----------------------------------------------------------------------------
This function muffles the current background music by reducing its volume to 
create a "muffled" effect, which can be used during certain gameplay scenarios.

This function was initially used for paused state, but taken out due to confusion
when applying audio settings. It can be repurposed for other scenarios that 
require a muffled audio effect.
-----------------------------------------------------------------------------*/
void AudioManager::MuffleMusic()
{
    if (!gIsMuffled)
    {
        preservedGameVol = gCurrTrack->GetVolume();
        gIsMuffled = true;
    }

    gCurrTrack->SetVolume(preservedGameVol * 0.55f);
    gCurrTrack->ApplyFinalVolume();
    RefreshAllMusicVolumes();
}
/*-----------------------------------------------------------------------------
This function restores the volume of the current background music to its 
original level, undoing the muffled effect applied by the MuffleMusic function.
-----------------------------------------------------------------------------*/
void AudioManager::UnmuffleMusic()
{
    gCurrTrack->SetVolume(preservedGameVol);
    gIsMuffled = false;
    RefreshAllMusicVolumes();
}
/*-----------------------------------------------------------------------------
This function refreshes the volumes of all active background music tracks by 
calling ApplyFinalVolume on each track that is currently active.
-----------------------------------------------------------------------------*/
void AudioManager::RefreshAllMusicVolumes()
{
    if (bossIntroMusic && bossIntroMusic->IsActive())
        bossIntroMusic->ApplyFinalVolume();

    if (bossFightMusic && bossFightMusic->IsActive())
        bossFightMusic->ApplyFinalVolume();

    if (gameMusic && gameMusic->IsActive())
        gameMusic->ApplyFinalVolume();

    if (gameOverMusic && gameOverMusic->IsActive())
        gameOverMusic->ApplyFinalVolume();

    if (victoryMusic && victoryMusic->IsActive())
        victoryMusic->ApplyFinalVolume();

    if (menuMusic && menuMusic->IsActive())
        menuMusic->ApplyFinalVolume();

    if (creditsMusic && creditsMusic->IsActive())
        creditsMusic->ApplyFinalVolume();

    if (trapLava && trapLava->IsActive())
        trapLava->ApplyFinalVolume();
}
/*-----------------------------------------------------------------------------
This function updates the audio for the lava trap based on the player's 
proximity to it. 

It checks the distance from the player to the closest lava trap and adjusts the 
volume of the lava trap music accordingly, creating a dynamic audio effect that 
increases in intensity as the player gets closer to the lava and decreases as 
they move away.
-----------------------------------------------------------------------------*/
void AudioManager::UpdateLavaAudio(const TrapManager& trapMgr, const Player& player)
{
    if (!trapLava || !trapLava->IsActive())
        return;

    float dist = trapMgr.GetClosestLavaDistance(player.GetPosition());

    if (dist < 0.0f)
    {
        trapLava->SetVolume(0.0f);
        return;
    }

    float maxDistance = 10.0f;
    float volume = 1.0f - (dist / maxDistance);
    volume = Clamp01(volume);

    // To not make it too quiet when the player is at the edge of the lava's range
    volume = 0.1f + 0.9f * volume;

    trapLava->SetVolume(volume);
    trapLava->ApplyFinalVolume();
}
/*-----------------------------------------------------------------------------
Sound Effects Control for the AudioManager class.

This section includes functions for playing sound effects in response to game 
events, applying the appropriate volume settings based on the global SFX 
volume and master volume. 

                             SOUND EFFECTS CONTROL
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
This function plays a sound effect using the provided SFXAudio instance, 
applying the appropriate volume settings based on the global SFX volume and 
master volume.
-----------------------------------------------------------------------------*/
void AudioManager::PlaySFX(SFXAudio const& sfx, f32 const& volume, f32 const& pitch)
{
    const AEAudio& audio = sfx.GetAudio();

    if (AEAudioIsValidAudio(audio))
    {
        AEAudioPlay(audio, gSFXGroup, volume, pitch, 0);
    }
}
/*-----------------------------------------------------------------------------
Init/Update/Exit functions for the AudioManager class.

This section includes the implementation of the Init function, which initializes 
the audio system by creating audio groups and loading all background music and 
sound effect files. 

It ensures that all audio resources are ready for use when the game starts, and 
applies the initial volume settings to the audio groups. 

The Update and Exit functions can be implemented as needed for managing audio 
state during the game loop and cleaning up resources when the game exits.

                               INIT/UPDATE/EXIT
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
This function initializes the AudioManager by creating audio groups for music 
and sound effects if they do not already exist, applying the initial volume 
settings to the groups, and loading all background music and sound effect files 
into their respective BGMAudio and SFXAudio instances.
-----------------------------------------------------------------------------*/
void AudioManager::Init()
{
    if (!AEAudioIsValidGroup(gMusicGroup))
        gMusicGroup = AEAudioCreateGroup();

    if (!AEAudioIsValidGroup(gSFXGroup))
        gSFXGroup = AEAudioCreateGroup();

    ApplyGroupVolumes();

    // Load BGM files here
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

    // Buff SFXs
    if (!buffRevealSFX)
        buffRevealSFX = std::make_unique<SFXAudio>("Assets/music/BuffRevealSFX.mp3");
    if (!buffHoverOnceSFX)
        buffHoverOnceSFX = std::make_unique<SFXAudio>("Assets/music/BuffHoverOnceSFX.mp3");
    if (!buffConfirmSFX)
        buffConfirmSFX = std::make_unique<SFXAudio>("Assets/music/BuffConfirmSFX.mp3");

    // UI SFXs
    if (!buttonClick)
        buttonClick = std::make_unique<SFXAudio>("Assets/music/click_but.mp3");
    if (!healthPickup)
        healthPickup = std::make_unique<SFXAudio>("Assets/music/HealthPickup.mp3");

    // Player SFXs
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
    if (!enemyHurtCrit)
        enemyHurtCrit = std::make_unique<SFXAudio>("Assets/music/EnemyHurtCrit.mp3");
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

    // Trap SFXs
    if (!trapPressurePlate)
        trapPressurePlate = std::make_unique<SFXAudio>("Assets/music/TrapPressurePlate.mp3");
    if (!trapSpikes)
        trapSpikes = std::make_unique<SFXAudio>("Assets/music/TrapSpikes.mp3");
}
/*-----------------------------------------------------------------------------
This function updates the audio state for the AudioManager, particularly 
handling the crossfading between background music tracks when a crossfade is 
in progress. 

It updates the fade timer, calculates the appropriate volumes for the fading 
out and fading in tracks using cosine and sine functions for a 
smooth transition, and applies the final volumes to the respective tracks.
-----------------------------------------------------------------------------*/
void AudioManager::Update()
{
    if (gIsCrossfading && gFadeOutTrack && gFadeInTrack)
    {
        gFadeTimer += static_cast<f32>(Time::GetInstance().GetDeltaTime());
        f32 t = (gFadeDuration > 0.0f) ? Clamp01(gFadeTimer / gFadeDuration) : 1.0f;

        std::cout << "t=" << t
            << " out=" << std::cos(t * PI_2)
            << " in=" << std::sin(t * PI_2) << "\n";

        if (t >= 1.0f)
        {
            // gFadeOutTrack->Stop();
            gFadeOutTrack->SetVolume(0.0f);
            gFadeInTrack->SetVolume(1.0f);

            gIsCrossfading = false;
            gFadeOutTrack = nullptr;
            gFadeInTrack = nullptr;
        }
        else
        {
            gFadeOutTrack->SetVolume(std::cos(t * PI_2));
            gFadeInTrack->SetVolume(std::sin(t * PI_2));
            gFadeOutTrack->ApplyFinalVolume();
            gFadeInTrack->ApplyFinalVolume();
        }
    }
}
/*-----------------------------------------------------------------------------
This function cleans up all audio resources used by the AudioManager when the 
game is exiting. It stops all music, resets the runtime state, and unloads all
audio files and groups to free up memory and ensure a clean shutdown of the
audio system.
-----------------------------------------------------------------------------*/
void AudioManager::Exit()
{
    StopAllMusic();
    ResetRuntimeState();

    // Reset background music
    bossIntroMusic.reset();
    bossFightMusic.reset();
    gameMusic.reset();
    gameOverMusic.reset();
    victoryMusic.reset();
    menuMusic.reset();
    creditsMusic.reset();

    // Reset lava ambient sound
    trapLava.reset();

    // Reset buff card SFXs
    buffRevealSFX.reset();
    buffHoverOnceSFX.reset();
    buffConfirmSFX.reset();

    // Reset UI SFXs
    buttonClick.reset();
    healthPickup.reset();

    // Reset player SFXs
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

    // Reset enemy SFXs
    enemyHurt.reset();
    enemyHurtCrit.reset();
    druidCast.reset();
    druidImpact.reset();
    druidDeath.reset();
    skeletonAttack.reset();
    skeletonDeath.reset();

    // Reset boss SFXs
    bossCharging.reset();
    bossProjectile.reset();
    bossTeleport.reset();
    bossSlash.reset();
    bossDeath.reset();

    // Reset trap SFXs
    trapSpikes.reset();
    trapPressurePlate.reset();

    // Reset music groups
    AEAudioUnloadAudioGroup(gMusicGroup);
    AEAudioUnloadAudioGroup(gSFXGroup);

    // Reset audio groups to default state to prevent dangling pointers on reinit
    gMusicGroup = {};
    gSFXGroup = {};
}

/*-----------------------------------------------------------------------------
Volume Control for the AudioManager class.

This section includes functions for setting and getting the master volume, 
music volume, and sound effects volume.

                                VOLUME CONTROL
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
This function sets the master volume for the audio system, clamping the input 
value between 0.0 and 1.0 to ensure it stays within a valid range.
-----------------------------------------------------------------------------*/
void AudioManager::SetMasterVolume(float v)
{
    gMasterVolume = Clamp01(v);
    ApplyGroupVolumes();
    RefreshAllMusicVolumes();
}
/*-----------------------------------------------------------------------------
This function sets the music volume for the audio system, clamping the input 
value between 0.0 and 1.0 to ensure it stays within a valid range.
-----------------------------------------------------------------------------*/
void AudioManager::SetMusicVolume(float v)
{
    gMusicVolume = Clamp01(v);
    ApplyGroupVolumes();
    RefreshAllMusicVolumes();
}
/*-----------------------------------------------------------------------------
This function sets the sound effects volume for the audio system, clamping the 
input value between 0.0 and 1.0 to ensure it stays within a valid range.
-----------------------------------------------------------------------------*/
void AudioManager::SetSFXVolume(float v)
{
    gSFXVolume = Clamp01(v);
    ApplyGroupVolumes();
    RefreshAllMusicVolumes();
}
/*-----------------------------------------------------------------------------
This function retrieves the current master volume setting for the audio system.
-----------------------------------------------------------------------------*/
float AudioManager::GetMasterVolume()
{
    return gMasterVolume;
}
/*-----------------------------------------------------------------------------
This function retrieves the current sound effects volume setting for the audio 
system.
-----------------------------------------------------------------------------*/
float AudioManager::GetSFXVolume()
{
    return gSFXVolume;
}
/*-----------------------------------------------------------------------------
This function retrieves the current music volume setting for the audio system.
-----------------------------------------------------------------------------*/
float AudioManager::GetMusicVolume()
{
    return gMusicVolume;
}

/*-----------------------------------------------------------------------------
Reset Helpers for the AudioManager class.

This section includes functions for stopping all music, resetting the runtime 
state of the AudioManager, and resetting the audio state for a game restart.

                                RESET HELPERS
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
This function stops all background music tracks that are currently active. It
checks each track and calls the Stop function if the track is valid, ensuring
that all music is halted when this function is called, such as during a game
restart or when exiting the game.
-----------------------------------------------------------------------------*/
void AudioManager::StopAllMusic()
{
    if (gameMusic)      gameMusic->Stop();
    if (bossIntroMusic) bossIntroMusic->Stop();
    if (bossFightMusic) bossFightMusic->Stop();
    if (gameOverMusic)  gameOverMusic->Stop();
    if (victoryMusic)   victoryMusic->Stop();
    if (menuMusic)      menuMusic->Stop();
    if (creditsMusic)   creditsMusic->Stop();
    if (trapLava)       trapLava->Stop();
}
/*-----------------------------------------------------------------------------
This function resets the runtime state of the AudioManager, including flags for
muffling, crossfading, and which tracks are currently playing. It also resets
the current track pointer and any flags related to boss music and sound effects.
-----------------------------------------------------------------------------*/
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
    playedBossIntroMusic = false;

    gCurrTrack = nullptr;

    AudioManager::playedBossDeathSFX = false;
    AudioManager::playedBossChargingSFX = false;
    AudioManager::playedBossTeleportSFX = false;
}
/*-----------------------------------------------------------------------------
This function resets the audio state for a game restart by stopping all music 
and resetting the runtime state of the AudioManager. It also sets the current 
track pointer to nullptr to ensure that no music is considered active when the 
game is restarted, allowing for a clean slate when the player starts a new game 
session.
-----------------------------------------------------------------------------*/
void AudioManager::ResetForRestart()
{
    StopAllMusic();
    ResetRuntimeState();
    gCurrTrack = nullptr;
}

/*-----------------------------------------------------------------------------
UI Helpers for the AudioManager class.

This section includes functions for playing UI-related sound effects, such as 
button clicks, which are triggered in response to user interactions with the 
game's user interface.

                                UI HELPERS
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
This function plays the button click sound effect when a UI button is clicked.
-----------------------------------------------------------------------------*/
void AudioManager::PlayButtonClick()
{
    if (buttonClick)
        PlaySFX(*buttonClick, 0.8f * GetSFXVolume());
}