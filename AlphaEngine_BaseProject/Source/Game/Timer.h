#pragma once
#include <string>
#include <vector>
#include <unordered_map>

struct Timer {
	std::string name = ""; // Name of this timer.
	float startTime = 0.0f; // Start time in seconds. Will automatically be set to reference elapsed time on creation.
	float endTime = 0.0f; // End time in seconds. Will automatically be set to reference start time + elapsed time on creation.
	float duration = 0.0f; // Duration of timer in seconds.
	float percentage = 0.0f; // Percentage of timer completed (0.0 to 1.0).
	bool completed = false; // Whether the timer has completed.
	bool autoRemove = true; // Whether to automatically remove the timer on completion.
	int completedCount = 0; // Number of times this timer has completed (for recurring timers).
};
class TimerSystem {
public:
	// Returns delta time in seconds.
	float GetDeltaTime() const;

	// Returns total elapsed time in seconds.
	float GetElapsedTime() const;

	// Updates the timer system, should be called once per frame.
	void Update();

	// Clears all timers from the system.
	void Clear();

	// Adds a new timer to the system.
	void AddTimer(const std::string& name, float duration, bool autoRemove);

	// Removes a timer from the system by name.
	void RemoveTimer(const std::string& name);

	// Returns a const reference to the list of timers.
	inline const std::vector<Timer>& GetTimers() const { return timers; }

	// Checks for completed timers and handles them.
	void CheckTimerCompletion();

	// Returns the count of active timers.
	inline int GetActiveTimerCount() const { return activeTimerCount; }

	// Resets the active timer count to zero.
	void ResetActiveTimerCount() { activeTimerCount = 0; }

	// Returns a pointer to a timer by name, or nullptr if not found.
	const Timer* GetTimerByName(const std::string& name) const;
/*_______________________________________________________________________________________*/
private:
	std::vector <Timer> timers; // Vector to store a list of timers.
	std::unordered_map<std::string, size_t> timerMap;  // Name -> index mapping to vector.
	float elapsedTime = 0.0f; // Total elapsed time in seconds.
	int activeTimerCount = 0; // Count of active timers.
};
