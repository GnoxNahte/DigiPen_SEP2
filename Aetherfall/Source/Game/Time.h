/*!
@file		Time.h
@author 	Wei Xiang NG
@brief		This header file declares the Time class for managing the game's 
            time system, including tracking elapsed time, delta time, and 
            controlling time scaling and pausing. The Time class is implemented
            as a singleton to allow easy access throughout the game code for 
            consistent time management across different systems and features.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
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
    f64 GetScaledDeltaTime() const;       // Last frame scaled delta

    // Time scale control
    void SetTimeScale(f32 scale);
    f32 GetTimeScale() const;

    // Pause control
    void SetPaused(bool paused);
    bool IsPaused() const;
    void TogglePause();

	// Reset all time values to zero (e.g. when starting a new game)
    void ResetElapsedTime();

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

