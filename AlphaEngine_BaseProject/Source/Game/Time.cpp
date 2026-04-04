/*!
@file		Time.cpp
@author 	Wei Xiang NG
@brief		This C++ handles the time system of the game, including tracking 
            elapsed time, delta time, and managing time scaling and pausing. 
            It provides functions to retrieve different types of time 
            (real-time, game time, unpaused time) and to control the flow of time 
            for gameplay effects such as slow motion or pausing. The Time class is 
            implemented as a singleton to allow easy access throughout the game code.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
#include "Time.h"
#include <iostream>

/*-----------------------------------------------------------------------------
This function updates the time system each frame. 

It retrieves the delta time for the current frame using 
AEFrameRateControllerGetFrameTime(), which provides the time elapsed since 
the last frame. 

The function then updates the total elapsed time, the unpaused elapsed time, 
and the scaled elapsed time based on whether the time is currently paused and 
the current time scale.
-----------------------------------------------------------------------------*/
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
/*-----------------------------------------------------------------------------
These functions provide access to the different types of time tracked by the 
Time class.
-----------------------------------------------------------------------------*/
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
/*-----------------------------------------------------------------------------
This function sets the time scale for the game, which affects how fast or slow
time progresses in the game. A time scale of 1.0 means normal speed, while 
values greater than 1.0 will speed up time and values between 0.0 and 1.0 will 
slow down time. 

The function also includes clamping to prevent unreasonable time scales and
prints the new time scale to the console for debugging purposes.
-----------------------------------------------------------------------------*/
void Time::SetTimeScale(f32 scale) {
    if (scale == timeScale) return;
    // Clamp to reasonable values
    if (scale < 0.0f) scale = 0.0f;
    if (scale > 10.0f) scale = 10.0f;

    timeScale = scale;
    std::cout << "Time scale set to " << timeScale << "x" << std::endl;
}
/*-----------------------------------------------------------------------------
This function retrieves the current time scale, which indicates how fast or 
slow time is progressing in the game. 

It simply returns the value of the timeScale member variable.
-----------------------------------------------------------------------------*/
f32 Time::GetTimeScale() const {
    return timeScale;
}
/*-----------------------------------------------------------------------------
This function sets the paused state of the time system. 

When paused, the game time will not progress, but real-time will continue to 
update. This allows for effects such as pausing the game or creating a 
freeze-frame effect while still allowing certain systems 
(like UI animations or timers) to continue based on real-time.
-----------------------------------------------------------------------------*/
void Time::SetPaused(bool paused) {
      isPaused = paused;
      std::cout << "Time system " << (isPaused ? "paused" : "unpaused") << std::endl;
}
/*-----------------------------------------------------------------------------
This function checks if the time system is currently paused.
-----------------------------------------------------------------------------*/
bool Time::IsPaused() const {
    return isPaused;
}
/*-----------------------------------------------------------------------------
This function toggles the paused state of the time system. 

If the time is currently paused, it will unpause it, and if it is currently 
unpaused, it will pause it.
-----------------------------------------------------------------------------*/
void Time::TogglePause() {
    SetPaused(!isPaused);
}
/*-----------------------------------------------------------------------------
This function resets all time values to zero, which can be useful when starting 
a new game or restarting the current game. 

It resets the elapsed time, scaled elapsed time, unpaused elapsed time, and 
delta time to zero, and it also unpauses the time system to ensure that time 
starts progressing immediately after the reset.
-----------------------------------------------------------------------------*/
void Time::ResetElapsedTime()
{
    elapsedTime = 0.0;
    scaledElapsedTime = 0.0;
    unpausedElapsedTime = 0.0;
    deltaTime = 0.0;
    isPaused = false;
}