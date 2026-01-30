#pragma once
#include "AEEngine.h"

class Time {
public:
    Time(const Time&) = delete;
    Time& operator=(const Time&) = delete;

    static Time& GetInstance() {
        static Time instance;
        return instance;
    }

    void Update();

    // Time getters
    f64 GetElapsedTime() const;           // Real-time
    f64 GetScaledElapsedTime() const;     // Game time
    f64 GetUnpausedElapsedTime() const;   // Unscaled but pausable
    f64 GetDeltaTime() const;             // Last frame delta

    // Time scale control
    void SetTimeScale(f32 scale);
    f32 GetTimeScale() const;

    // Pause control
    void SetPaused(bool paused);
    bool IsPaused() const;
    void TogglePause();

private:

    Time() :
        elapsedTime(0.0),
        scaledElapsedTime(0.0),
        unpausedElapsedTime(0.0),
        deltaTime(0.0),
        timeScale(1.0f),
        isPaused(false) {
    }

    f64 elapsedTime;           // Real-time (never pauses, never scales)
    f64 scaledElapsedTime;     // Game time (pauses and scales)
    f64 unpausedElapsedTime;   // Unscaled but pausable
    f64 deltaTime;             // Last frame's delta time

    f32 timeScale;
    bool isPaused;
};

