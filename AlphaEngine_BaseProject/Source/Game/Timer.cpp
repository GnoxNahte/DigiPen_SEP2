#include "Timer.h"
#include "Time.h"
#include <iostream>

void TimerSystem::Update() {
	CheckTimerCompletion();
}

void TimerSystem::Clear() {
	std::cout << "Clearing all timers from TimerSystem." << std::endl;
	timers.clear();
	timerMap.clear();
	anonymousTimerMap.clear();
	ResetActiveTimerCount();
}

void TimerSystem::AddTimer(const std::string& name, f64 duration, bool autoRemove,
	bool ignoreTimeScale, bool ignorePause, bool loopable, u32 loopCount) {
	Timer timer;
	if ((TimerSystem::GetTimerByName(name)) != nullptr) {
		std::cout << "Timer \"" << name << "\" already exists. Skipping addition." << std::endl;
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

	std::cout << "Initialized Timer \"" << timer.name << "\" for " << timer.duration
		<< " seconds (ignoreScale=" << ignoreTimeScale
		<< ", ignorePause=" << ignorePause << ")" << std::endl;
}
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
		std::cout << "Removed Timer \"" << name << "\"." << std::endl;
	}
}


const Timer* TimerSystem::GetTimerByName(const std::string& name) const {
	auto it = timerMap.find(name);
	if (it != timerMap.end()) {
		return &timers[it->second];
	}
	return nullptr; // Return nullptr if timer not found
}

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

	std::cout << "Initialized Anonymous Timer ID:" << timer.id
		<< " for " << timer.duration << " seconds"
		<< " (ignoreScale=" << ignoreTimeScale
		<< ", ignorePause=" << ignorePause << ")" << std::endl;

	return timer.id;
}

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
		std::cout << "Removed Anonymous Timer ID:" << timerId << std::endl;
	}
}

const Timer* TimerSystem::GetTimerById(u32 timerId) const {
	auto it = anonymousTimerMap.find(timerId);
	if (it != anonymousTimerMap.end()) {
		return &timers[it->second];
	}
	return nullptr;
}

bool TimerSystem::IsTimerComplete(u32 timerId) const {
	const Timer* timer = GetTimerById(timerId);
	return timer ? timer->completed : false;
}

f32 TimerSystem::GetTimerPercentage(u32 timerId) const {
	const Timer* timer = GetTimerById(timerId);
	return timer ? (static_cast<f32>(timer->percentage)) : 0.0f;
}

// Helper to get the right time counter for a timer
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
			std::cout << "Timer ";
			if (it->isAnonymous) {
				std::cout << "ID:" << it->id;
			}
			else {
				std::cout << "\"" << it->name << "\"";
			}
			std::cout << " completed!" << std::endl;

			if (it->autoRemove) {
				activeTimerCount--;
				size_t index = std::distance(timers.begin(), it);

				if (it->isAnonymous) {
					anonymousTimerMap.erase(it->id);
				}
				else {
					std::cout << "Removed Timer \"" << it->name << "\"." << std::endl;
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

