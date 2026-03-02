#include "Time.h"
#include <iostream>

void Time::Update() {
    deltaTime = AEFrameRateControllerGetFrameTime();

    // Always update real-time
    elapsedTime += deltaTime;

    // Update unpaused unscaled time
    if (!isPaused) {
        unpausedElapsedTime += deltaTime;
    }

    // Update scaled pausable time (game time)
    if (!isPaused) {
        scaledElapsedTime += deltaTime * timeScale;
    }
}

f64 Time::GetElapsedTime() const {
    return elapsedTime;
}

f64 Time::GetScaledElapsedTime() const {
    return scaledElapsedTime;
}

f64 Time::GetUnpausedElapsedTime() const {
    return unpausedElapsedTime;
}

f64 Time::GetDeltaTime() const {
    return deltaTime;
}

f64 Time::GetScaledDeltaTime() const
{
    return deltaTime * timeScale;
}

void Time::SetTimeScale(f32 scale) {
    // Clamp to reasonable values
    if (scale < 0.0f) scale = 0.0f;
    if (scale > 10.0f) scale = 10.0f;

    timeScale = scale;
    std::cout << "Time scale set to " << timeScale << "x" << std::endl;
}

f32 Time::GetTimeScale() const {
    return timeScale;
}

void Time::SetPaused(bool paused) {
      isPaused = paused;
      std::cout << "Time system " << (isPaused ? "paused" : "unpaused") << std::endl;
}

bool Time::IsPaused() const {
    return isPaused;
}

void Time::TogglePause() {
    SetPaused(!isPaused);
}

void Time::ResetElapsedTime()
{
    elapsedTime = 0.0;
    scaledElapsedTime = 0.0;
    unpausedElapsedTime = 0.0;
    deltaTime = 0.0;
    isPaused = false;
}