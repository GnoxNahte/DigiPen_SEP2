#pragma once
#include "AEEngine.h"
#include "../Game/enemy/EnemyBoss.h"
#include "../Game/Rooms/RoomManager.h"

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
    static void Exit();

    static void SetMasterVolume(float v);
    static void SetSFXVolume(float v);
    static void SetMusicVolume(float v);

    static float GetMasterVolume();
    static float GetSFXVolume();
    static float GetMusicVolume();

    static void PlaySFX(SFXAudio const& sfx, f32 const& pitch = 1.0f);
    static void PlayBossMusic(EnemyBoss const& boss, RoomManager const& roomMgr);
    static void MuffleGameMusic();
    static void UnmuffleGameMusic();
    static void RefreshAllMusicVolumes();


    // Background Music
    static std::unique_ptr<BGMAudio> bossIntroMusic;
    static std::unique_ptr<BGMAudio> bossFightMusic;
    static std::unique_ptr<BGMAudio> gameMusic;
    static std::unique_ptr<BGMAudio> bossInstrMusic;
    //static std::unique_ptr<BGMAudio> menuMusic;

    // Sound Effects
    //static std::unique_ptr<SFXAudio> buffFlipSFX;
    static std::unique_ptr<SFXAudio> buffRevealSFX;
    static std::unique_ptr<SFXAudio> buffHoverOnceSFX;
    static std::unique_ptr<SFXAudio> buffConfirmSFX;
};