#pragma once
#include "AEEngine.h"

enum class SoundId
{
    PlayerAttack1,
    PlayerAttack2,
    PlayerAttack3
};

enum class MusicId
{
    MainMenu,
    Death,
    GameScene,
    BossIntro,
    BossCombat,
    Victory
};

struct Audio {
    AEAudio audioFile{};
    f32 volume{}, pitch{};
    bool active{};
};

struct MusicChannel
{
    MusicId id;
    float volume;
    int handle;
    bool playing;
};


class AudioManager
{
public:
    static void Init();
    static void Update();
    static void LoadAll();
    static void Exit();

    static void SetMasterVolume(float v);
    static void SetSFXVolume(float v);
    static void SetMusicVolume(float v);

    static float GetMasterVolume();
    static float GetSFXVolume();
    static float GetMusicVolume();

    static void PlaySFX(SoundId id, float volumeScale = 1.0f, float pitch = 1.0f, int loopCount = 0);
    static void PlayMusic(MusicId id, float volumeScale = 1.0f, float pitch = 1.0f, int loopCount = 0);

    static void PlayNextAttackSFX();
    static void Crossfade(Audio& from, Audio& to, f32 duration);
};