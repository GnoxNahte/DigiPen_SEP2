#pragma once
#include "AEEngine.h"
#include "../Game/enemy/EnemyBoss.h"
#include "../Game/Rooms/RoomManager.h"
#include "../Game/Player/Player.h"
#include "../Game/Environment/traps.h"

class BGMAudio {
public:
    BGMAudio() = delete;
    BGMAudio(char const* filename);
    ~BGMAudio();
    void Play(f32 const& initialGroupVol);
    void Stop();
    void SetVolume(f32 const& vol);
    const f32& GetVolume() const { return baseVolume; }
    void ApplyFinalVolume();
    const f32& GetPitch() const { return pitch; }
    const bool& IsActive() const { return active; }
    void SetActive(bool activeState) { active = activeState; }
    void CrossfadeTo(BGMAudio& other, f32 duration);

private:
    AEAudio audioFile{};
    AEAudioGroup ownGroup{};  // each instance owns its group
    f32 baseVolume{}, fadeVolume{}, pitch{};
    bool active{};
};

class SFXAudio {
public:
    SFXAudio() = delete;
    SFXAudio(char const* filename);
    ~SFXAudio();
    AEAudio const& GetAudio() const { return audioFile; }
private:
    AEAudio audioFile{};
};


class AudioManager
{
public:
    static void Init();
    static void Update();
    static void Exit();

    static void SetMasterVolume(float v);
    static void SetSFXVolume(float v);
    static void SetMusicVolume(float v);

    static float GetMasterVolume();
    static float GetSFXVolume();
    static float GetMusicVolume();

    static void PlaySFX(SFXAudio const& sfx, f32 const& volume, f32 const& pitch = 1.0f);
    static void PlayBossMusic(EnemyBoss const& boss, RoomManager const& roomMgr);
    static void PlayMenuMusic();
    static void PlayGameMusic();
    static void PlayGameOverMusic();
    static void MuffleMusic();
    static void UnmuffleMusic();
    static void RefreshAllMusicVolumes();
    static void UpdateLavaAudio(const TrapManager& trapMgr, const Player& player);

	// UI Audio for buttons
    static void PlayButtonClick();


    // Flags
	inline static bool playedBossIntroMusic = false;
    inline static bool playedBossChargingSFX = false;
    inline static bool playedBossTeleportSFX = false;
    inline static bool playedBossDeathSFX = false;
    inline static bool playedDruidImpactSFX = false;

    // Reset functions
    static void StopAllMusic();
    static void ResetRuntimeState();
    static void ResetForRestart();

    /*=================================================
    *                Background Music                 |
    =================================================*/
    static std::unique_ptr<BGMAudio> bossIntroMusic;
    static std::unique_ptr<BGMAudio> bossFightMusic;
    static std::unique_ptr<BGMAudio> gameMusic;
    static std::unique_ptr<BGMAudio> gameOverMusic;
    static std::unique_ptr<BGMAudio> victoryMusic;
    static std::unique_ptr<BGMAudio> menuMusic;
    static std::unique_ptr<BGMAudio> creditsMusic;

    static std::unique_ptr<BGMAudio> trapLava;

    /*=================================================
    *                 Sound Effects                   |
    =================================================*/
    // Buff SFXs
    static std::unique_ptr<SFXAudio> buffRevealSFX;
    static std::unique_ptr<SFXAudio> buffHoverOnceSFX;
    static std::unique_ptr<SFXAudio> buffConfirmSFX;

    // UI SFXs
    static std::unique_ptr<SFXAudio> buttonClick;

    // Player SFXs
    static std::unique_ptr<SFXAudio> playerAttack1;
    static std::unique_ptr<SFXAudio> playerAttack2;
    static std::unique_ptr<SFXAudio> playerAttack3;
    static std::unique_ptr<SFXAudio> playerAirAttack;
    static std::unique_ptr<SFXAudio> playerAirAttackImpact;
    static std::unique_ptr<SFXAudio> playerJump;
    static std::unique_ptr<SFXAudio> playerLand;
    static std::unique_ptr<SFXAudio> playerDash;
    static std::unique_ptr<SFXAudio> playerHurt;
    static std::unique_ptr<SFXAudio> playerDeath;

    // Enemy SFXs
    static std::unique_ptr<SFXAudio> enemyHurt;
    static std::unique_ptr<SFXAudio> enemyHurtCrit;
    static std::unique_ptr<SFXAudio> druidCast;
    static std::unique_ptr<SFXAudio> druidImpact;
    static std::unique_ptr<SFXAudio> druidDeath;
    static std::unique_ptr<SFXAudio> skeletonAttack;
    static std::unique_ptr<SFXAudio> skeletonDeath;

    // Boss SFXs
    static std::unique_ptr<SFXAudio> bossCharging;
    static std::unique_ptr<SFXAudio> bossProjectile;
    static std::unique_ptr<SFXAudio> bossTeleport;
    static std::unique_ptr<SFXAudio> bossSlash;
    static std::unique_ptr<SFXAudio> bossDeath;

    // Trap SFXs
    static std::unique_ptr<SFXAudio> trapPressurePlate;
    static std::unique_ptr<SFXAudio> trapSpikes;
};