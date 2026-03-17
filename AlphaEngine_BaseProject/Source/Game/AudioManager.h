#pragma once
#include "AEEngine.h"
#include "../Game/enemy/EnemyBoss.h"
#include "../Game/Rooms/RoomManager.h"


// Sound Effects.
//enum class SoundId
//{
//    PlayerAttack1,
//    PlayerAttack2,
//    PlayerAttack3
//};

// MusicId KIV whether to be used.
//enum class MusicId
//{
//    MainMenu,
//    Death,
//    GameScene,
//    BossIntro,
//    BossCombat,
//    Victory
//};

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

//struct MusicChannel
//{
//    MusicId id;
//    float volume;
//    int handle;
//    bool playing;
//};


class AudioManager
{
public:
    static void Init();
    static void Update();
    //static void LoadAll();
    static void Exit();

    static void SetMasterVolume(float v);
    static void SetSFXVolume(float v);
    static void SetMusicVolume(float v);

    static float GetMasterVolume();
    static float GetSFXVolume();
    static float GetMusicVolume();

    static void InitializeBossMusic(EnemyBoss const& boss, RoomManager const& roomMgr);
    static void RefreshAllMusicVolumes();

    static std::unique_ptr<BGMAudio> bossIntroMusic;
    static std::unique_ptr<BGMAudio> bossFightMusic;

    //static void PlaySFX(SoundId id, float volumeScale = 1.0f, float pitch = 1.0f, int loopCount = 0);
    //static void PlayMusic(MusicId id, float volumeScale = 1.0f, float pitch = 1.0f, int loopCount = 0);

    //static void PlayNextAttackSFX();
    //static void Crossfade(Audio& from, Audio& to, f32 duration);
};