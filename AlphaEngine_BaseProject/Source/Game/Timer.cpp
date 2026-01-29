#include "Timer.h"
#include <iostream>

// Initialize static instance pointer
TimerSystem* TimerSystem::instance = nullptr;

TimerSystem* TimerSystem::GetInstance() {
	if (instance == nullptr) {
		instance = new TimerSystem();
		std::cout << "TimerSystem singleton created." << std::endl;
	}
	return instance;
}

void TimerSystem::DestroyInstance() {
	if (instance != nullptr) {
		std::cout << "TimerSystem singleton destroyed." << std::endl;
		delete instance;
		instance = nullptr;
	}
}

f64 TimerSystem::GetElapsedTime() const {
	return elapsedTime;
}
void TimerSystem::Update() {
	f64 dt = AEFrameRateControllerGetFrameTime();
	elapsedTime += dt;
	CheckTimerCompletion();
}

void TimerSystem::AddTimer(const std::string& name, f64 duration, bool autoRemove) {
	Timer timer;
	if ((TimerSystem::GetTimerByName(name)) != nullptr) {
		std::cout << "Timer \"" << name << "\" already exists. Skipping addition." << std::endl;
		return;
	}
	timer.name = name;
	timer.startTime = elapsedTime;
	timer.endTime = timer.startTime + duration;
	timer.duration = duration;
	timer.percentage = 0.0f;
	timer.completed = false;
	timer.autoRemove = autoRemove;
	timer.completedCount = 0;

	timers.push_back(timer);
	timerMap[name] = timers.size() - 1;
	activeTimerCount++;
	std::cout << "Initialized Timer \"" << timer.name << "\" for " << timer.duration << " seconds." << std::endl;
}
void TimerSystem::RemoveTimer(const std::string& name) {
	auto it = timerMap.find(name);
	if (it != timerMap.end()) {
		size_t index = it->second;
		timers.erase(timers.begin() + index);
		timerMap.erase(it);
		// Update all indices after the erased element for map
		for (auto& pair : timerMap) {
			if (pair.second > index) {
				pair.second--;
			}
		}
		activeTimerCount--;
		std::cout << "Removed Timer \"" << name << "\"." << std::endl;
	}
}

void TimerSystem::CheckTimerCompletion() {
	for (auto it = timers.begin(); it != timers.end(); ) {
		it->percentage = (elapsedTime - it->startTime) / it->duration;
		if (it->percentage > 1.0f) {
			it->percentage = 1.0f;
		}

		if (elapsedTime >= it->endTime) {
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

				// Remove from appropriate map
				if (it->isAnonymous) {
					anonymousTimerMap.erase(it->id);
				}
				else {
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
			else {
				it->startTime = elapsedTime;
				it->endTime = it->startTime + it->duration;
				it->percentage = 0.0f;
				it->completed = false;
				it->completedCount++;
			}
		}
		++it;
	}
}

void TimerSystem::Clear() {
	std::cout << "Clearing all timers from TimerSystem." << std::endl;
	timers.clear();
	timerMap.clear();
	ResetActiveTimerCount();
}

const Timer* TimerSystem::GetTimerByName(const std::string& name) const {
	auto it = timerMap.find(name);
	if (it != timerMap.end()) {
		return &timers[it->second];
	}
	return nullptr; // Return nullptr if timer not found
}

u32 TimerSystem::AddAnonymousTimer(f64 duration, bool autoRemove) {
	Timer timer;
	timer.id = nextTimerId++;
	timer.name = ""; // Empty name for anonymous
	timer.isAnonymous = true;
	timer.startTime = elapsedTime;
	timer.endTime = timer.startTime + duration;
	timer.duration = duration;
	timer.percentage = 0.0f;
	timer.completed = false;
	timer.autoRemove = autoRemove;
	timer.completedCount = 0;

	timers.push_back(timer);
	anonymousTimerMap[timer.id] = timers.size() - 1;
	activeTimerCount++;

	std::cout << "Initialized Anonymous Timer ID:" << timer.id
		<< " for " << timer.duration << " seconds." << std::endl;

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