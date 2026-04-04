/*!
@file		Timer.cpp
@author 	Wei Xiang NG
@brief		This C++ file implements the TimerSystem class for managing timers in the game, including 
			both named and anonymous timers. The TimerSystem allows for creating timers with various 
			settings (e.g. ignoring time scale or pause state), checking their completion status, and 
			retrieving their progress. It is designed to be updated every frame to handle timer completion
			and looping logic, and provides functions for adding, removing, and querying timers by name or ID.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
#include "Timer.h"
#include "Time.h"
#include <iostream>

/*-----------------------------------------------------------------------------
This function checks for completed timers and handles them accordingly. 

It iterates through the list of timers and checks if any have reached their 
end time based on the appropriate time base (real-time, scaled time, 
or unpaused time) depending on their settings. 

If a timer has completed, it updates its completion status, percentage, 
and completed count. For loopable timers, it resets the start and end times
to loop again. 

For non-loopable timers that are set to auto-remove, it removes them from the 
system. This function is called every frame in the Update() function to ensure 
timely handling of timer completions.
-----------------------------------------------------------------------------*/
void TimerSystem::Update() {
	CheckTimerCompletion();
}
/*-----------------------------------------------------------------------------
This function clears all timers from the TimerSystem. 

It empties the list of timers and the associated maps for named and 
anonymous timers, effectively resetting the timer system to an empty state.
-----------------------------------------------------------------------------*/
void TimerSystem::Clear() {
	std::cout << "Clearing all timers from TimerSystem." << std::endl;
	timers.clear();
	timerMap.clear();
	anonymousTimerMap.clear();
	ResetActiveTimerCount();
}
/*-----------------------------------------------------------------------------
This function adds a new timer to the TimerSystem with the specified settings.

It first checks if a timer with the same name already exists (for named timers) 
and skips addition if it does.
-----------------------------------------------------------------------------*/
void TimerSystem::AddTimer(const std::string& name, f64 duration, bool autoRemove,
	bool ignoreTimeScale, bool ignorePause, bool loopable, u32 loopCount) {
	Timer timer;
	if ((TimerSystem::GetTimerByName(name)) != nullptr) {
		//std::cout << "Timer \"" << name << "\" already exists. Skipping addition." << std::endl;
		return;
	}
	timer.name = name;
	timer.isAnonymous = false;
	timer.id = 0;
	timer.ignoreTimeScale = ignoreTimeScale;
	timer.ignorePause = ignorePause;

	// Use correct time based on timer settings
	f64 currentTime = GetTimeForTimer(timer);

	timer.startTime = currentTime;
	timer.endTime = timer.startTime + duration;
	timer.duration = duration;
	timer.percentage = 0.0f;
	timer.completed = false;
	timer.autoRemove = autoRemove;
	timer.completedCount = 0;
	timer.loopable = loopable;
	timer.loopCount = loopCount;

	timers.push_back(timer);
	timerMap[name] = timers.size() - 1;
	activeTimerCount++;

	//std::cout << "Initialized Timer \"" << timer.name << "\" for " << timer.duration
	//	<< " seconds (ignoreScale=" << ignoreTimeScale
	//	<< ", ignorePause=" << ignorePause << ")" << std::endl;
}
/*-----------------------------------------------------------------------------
This function removes a timer from the TimerSystem by its name.
-----------------------------------------------------------------------------*/
void TimerSystem::RemoveTimer(const std::string& name) {
	auto it = timerMap.find(name);
	if (it != timerMap.end()) {
		size_t index = it->second;
		timers.erase(timers.begin() + index);
		timerMap.erase(it);

		for (auto& pair : timerMap) {
			if (pair.second > index) {
				pair.second--;
			}
		}
		for (auto& pair : anonymousTimerMap) {
			if (pair.second > index) {
				pair.second--;
			}
		}

		activeTimerCount--;
		//std::cout << "Removed Timer \"" << name << "\"." << std::endl;
	}
}
/*-----------------------------------------------------------------------------
This function retrieves a pointer to a timer by its name. 
If the timer is found, it returns a pointer to it; otherwise, it returns 
nullptr.
-----------------------------------------------------------------------------*/
const Timer* TimerSystem::GetTimerByName(const std::string& name) const {
	auto it = timerMap.find(name);
	if (it != timerMap.end()) {
		return &timers[it->second];
	}
	return nullptr; // Return nullptr if timer not found
}
/*-----------------------------------------------------------------------------
This function adds an anonymous timer to the TimerSystem with the specified 
settings and returns its unique ID.
-----------------------------------------------------------------------------*/
u32 TimerSystem::AddAnonymousTimer(f64 duration, bool autoRemove,
	bool ignoreTimeScale, bool ignorePause, bool loopable, u32 loopCount) {
	Timer timer;
	timer.id = nextTimerId++;
	timer.name = "";
	timer.isAnonymous = true;
	timer.ignoreTimeScale = ignoreTimeScale;
	timer.ignorePause = ignorePause;

	// Use appropriate time base from Time system
	f64 currentTime = GetTimeForTimer(timer);

	timer.startTime = currentTime;
	timer.endTime = timer.startTime + duration;
	timer.duration = duration;
	timer.percentage = 0.0f;
	timer.completed = false;
	timer.autoRemove = autoRemove;
	timer.completedCount = 0;
	timer.loopable = loopable;
	timer.loopCount = loopCount;

	timers.push_back(timer);
	anonymousTimerMap[timer.id] = timers.size() - 1;
	activeTimerCount++;

	/*std::cout << "Initialized Anonymous Timer ID:" << timer.id
		<< " for " << timer.duration << " seconds"
		<< " (ignoreScale=" << ignoreTimeScale
		<< ", ignorePause=" << ignorePause << ")" << std::endl;*/

	return timer.id;
}
/*-----------------------------------------------------------------------------
This function removes an anonymous timer from the TimerSystem by its unique ID.
-----------------------------------------------------------------------------*/
void TimerSystem::RemoveAnonymousTimer(u32 timerId) {
	auto it = anonymousTimerMap.find(timerId);
	if (it != anonymousTimerMap.end()) {
		size_t index = it->second;
		timers.erase(timers.begin() + index);
		anonymousTimerMap.erase(it);

		// Update all indices after the erased element
		for (auto& pair : anonymousTimerMap) {
			if (pair.second > index) {
				pair.second--;
			}
		}
		// Also update named timer map
		for (auto& pair : timerMap) {
			if (pair.second > index) {
				pair.second--;
			}
		}

		activeTimerCount--;
		//std::cout << "Removed Anonymous Timer ID:" << timerId << std::endl;
	}
}
/*-----------------------------------------------------------------------------
This function retrieves a pointer to an anonymous timer by its unique ID.
-----------------------------------------------------------------------------*/
const Timer* TimerSystem::GetTimerById(u32 timerId) const {
	auto it = anonymousTimerMap.find(timerId);
	if (it != anonymousTimerMap.end()) {
		return &timers[it->second];
	}
	return nullptr;
}
/*-----------------------------------------------------------------------------
This function checks if an anonymous timer has completed by its unique ID. 
It returns true if the timer is found and has completed, or false if the timer 
is not found or has not completed.
-----------------------------------------------------------------------------*/
bool TimerSystem::IsTimerComplete(u32 timerId) const {
	const Timer* timer = GetTimerById(timerId);
	return timer ? timer->completed : false;
}
/*-----------------------------------------------------------------------------
This function retrieves the percentage of completion for an anonymous timer 
by its unique ID.
-----------------------------------------------------------------------------*/
f32 TimerSystem::GetTimerPercentage(u32 timerId) const {
	const Timer* timer = GetTimerById(timerId);
	return timer ? (static_cast<f32>(timer->percentage)) : 0.0f;
}
/*-----------------------------------------------------------------------------
This helper function determines the appropriate time base to use for a given 
timer based on its settings for ignoring time scale and pause state. 

It returns the current time from the Time system that should be used for 
calculating the timer's progress and completion status, ensuring that timers
behave correctly according to their configuration (e.g. real-time, paused, 
or scaled).
-----------------------------------------------------------------------------*/
f64 TimerSystem::GetTimeForTimer(const Timer& timer) const {
	if (timer.ignorePause && timer.ignoreTimeScale) {
		// Real-time: never pauses, never scales
		return Time::GetInstance().GetElapsedTime();
	}
	else if (!timer.ignorePause && timer.ignoreTimeScale) {
		// Pauses but doesn't scale
		return Time::GetInstance().GetUnpausedElapsedTime();
	}
	else {
		// Normal timer: pauses and scales
		return Time::GetInstance().GetScaledElapsedTime();
	}
}
/*-----------------------------------------------------------------------------
This function checks for completed timers and handles them accordingly. 

It iterates through the list of timers and checks if any have reached their end
time based on the appropriate time base (real-time, scaled time, or unpaused time) 
depending on their settings. 

If a timer has completed, it updates its completion status, percentage, and 
completed count. For loopable timers, it resets the start and end times to loop 
again.
-----------------------------------------------------------------------------*/
void TimerSystem::CheckTimerCompletion() {
	for (auto it = timers.begin(); it != timers.end(); ) {
		// Get the appropriate time counter for this timer
		f64 currentTime = GetTimeForTimer(*it);

		// Calculate percentage
		it->percentage = (currentTime - it->startTime) / it->duration;
		if (it->percentage > 1.0f) {
			it->percentage = 1.0f;
		}

		// Check completion
		if (currentTime >= it->endTime) {
			it->completed = true;
			/*std::cout << "Timer ";
			if (it->isAnonymous) {
				std::cout << "ID:" << it->id;
			}
			else {
				std::cout << "\"" << it->name << "\"";
			}
			std::cout << " completed!" << std::endl;*/

			if (it->autoRemove) {
				activeTimerCount--;
				size_t index = std::distance(timers.begin(), it);

				if (it->isAnonymous) {
					anonymousTimerMap.erase(it->id);
				}
				else {
					//std::cout << "Removed Timer \"" << it->name << "\"." << std::endl;
					timerMap.erase(it->name);
				}

				it = timers.erase(it);

				// Update both maps
				for (auto& pair : timerMap) {
					if (pair.second > index) {
						pair.second--;
					}
				}
				for (auto& pair : anonymousTimerMap) {
					if (pair.second > index) {
						pair.second--;
					}
				}
				continue;
			}
			// Handle looping timers, loopCount is reduced by 1 to account for initial completion
			else if (it->loopable && it->completedCount < it->loopCount - 1){
				// Loop timer - restart with current time
				it->startTime = currentTime;
				it->endTime = it->startTime + it->duration;
				it->percentage = 0.0f;
				it->completed = false;
				it->completedCount++;
			}
		}
		++it;
	}
}

