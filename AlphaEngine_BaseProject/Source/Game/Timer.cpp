#include "Timer.h"
#include <iostream>

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
			std::cout << "Timer \"" << it->name << "\" completed!" << std::endl;

			// Auto remove the timer from the system if enabled
			if (it->autoRemove) {
				activeTimerCount--;
				std::cout << "Removed Timer \"" << it->name << "\"." << std::endl;
				// Rebuild map after erasing
				size_t index = std::distance(timers.begin(), it);
				timerMap.erase(it->name);
				it = timers.erase(it);

				// Update all indices after the erased element
				for (auto& pair : timerMap) {
					if (pair.second > index) {
						pair.second--;
					}
				}
				continue;
			}
			// Auto remove turned off, the timer will loop until caller uses RemoveTimer.
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