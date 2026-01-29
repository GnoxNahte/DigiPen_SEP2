#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <AEEngine.h>

struct Timer {
	std::string name = ""; // Name of this timer.
	f64 startTime = 0.0f; // Start time in seconds. Will automatically be set to reference elapsed time on creation.
	f64 endTime = 0.0f; // End time in seconds. Will automatically be set to reference start time + elapsed time on creation.
	f64 duration = 0.0f; // Duration of timer in seconds.
	f64 percentage = 0.0f; // Percentage of timer completed (0.0 to 1.0).
	bool completed = false; // Whether the timer has completed.
	bool autoRemove = true; // Whether to automatically remove the timer on completion. True by default.
	u32 completedCount = 0; // Number of times this timer has completed (for recurring timers).
	u32 id = 0; // Unique identifier for the timer.
	bool isAnonymous = false; // Whether the timer is anonymous (no name).
	bool ignoreTimeScale; // Whether the timer ignores time scale.
	bool ignorePause; // Whether the timer ignores pause state.
	bool loopable; // Whether the timer loops upon completion.
	u32 loopCount = 0; // Number of times the timer has looped.
};
class TimerSystem {
public:
	// Get singleton instance
	static TimerSystem* GetInstance();

	// Destroy singleton (On shutdown)
	static void DestroyInstance();

	// Returns total elapsed time in seconds.
	//f64 GetElapsedTime() const;

	// Updates the timer system, should be called once per frame.
	void Update();

	// Clears all timers from the system.
	void Clear();

	// ========= NAMED TIMERS ========= //
	
	// Adds a new timer to the system.
	void AddTimer(const std::string& name, f64 duration, bool autoRemove = true,
		bool ignoreTimeScale = false, bool ignorePause = false, bool loopable = false, u32 loopCount = 0);

	// Removes a timer from the system by name.
	void RemoveTimer(const std::string& name);

	// Returns a pointer to a timer by name, or nullptr if not found.
	const Timer* GetTimerByName(const std::string& name) const;

	// Returns a const reference to the list of timers.
	//const std::vector<Timer>& GetTimers() const { return timers; }

	// ========= ANONYMOUS TIMERS ========= //

	// Add anonymous timer - returns handle/ID
	u32 AddAnonymousTimer(f64 duration, bool autoRemove = true,
		bool ignoreTimeScale = false, bool ignorePause = false, bool loopable = false, u32 loopCount = 0);

	// Remove anonymous timer by ID
	void RemoveAnonymousTimer(u32 timerId);

	// Get anonymous timer by ID
	const Timer* GetTimerById(u32 timerId) const;

	// Check if anonymous timer is complete
	bool IsTimerComplete(u32 timerId) const;

	// Get percentage of anonymous timer
	f32 GetTimerPercentage(u32 timerId) const;
	
	// ========= TIMER MANAGEMENT ========= //

	// Returns the count of active timers.
	int GetActiveTimerCount() const { return activeTimerCount; }

	// Resets the active timer count to zero.
	void ResetActiveTimerCount() { activeTimerCount = 0; }
/*_______________________________________________________________________________________*/
private:
	static TimerSystem* instance;
	// Private constructor to prevent direct instantiation
	TimerSystem() : elapsedTime(0.0), activeTimerCount(0), nextTimerId(1) {}
	// Delete copy constructor and assignment operator
	TimerSystem(const TimerSystem&) = delete;
	TimerSystem& operator=(const TimerSystem&) = delete;

	// Member variables
	std::vector <Timer> timers; // Vector to store a list of timers.
	std::unordered_map<std::string, size_t> timerMap;  // Name -> index mapping to vector.
	std::unordered_map<u32, size_t> anonymousTimerMap; // For anonymous timers
	f64 elapsedTime = 0.0f; // Total elapsed time in seconds.
	int activeTimerCount = 0; // Count of active timers.
	u32 nextTimerId = 1; // Auto-incrementing ID for anonymous timers

	// Checks for completed timers and handles them.
	void CheckTimerCompletion();

	// Helper to get appropriate time for timer.
	f64 GetTimeForTimer(const Timer& timer) const;
};
