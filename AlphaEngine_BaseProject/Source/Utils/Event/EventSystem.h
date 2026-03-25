#pragma once

#include <functional>

using EventId = unsigned long;

class EventSystem
{
public:
	/**
	 * @brief			Subscribes a callback function to a specific event type
	 * @tparam T		The unique event type used for identification and callback data
	 * @param callback	Function executed when event T occurs. Receives T as data.
	 * @return			Return a EventId which is used to unsubcribe. Store this!
	 */
	template<typename T>
	static EventId Subscribe(std::function<void(const T&)> callback)
	{
		EventId id = nextId++;
		auto& listeners = GetListeners<T>();
		listeners.emplace(id, callback);
		return id;
	}

	/**
	 * @brief		Removes a subscriber from a specific event type.
	 * @tparam T	The event identifier type to unsubscribe from
	 * @param id	The EventId returned by Subscribe
	 */
	template<typename T>
	static void Unsubscribe(EventId id)
	{
		GetListeners<T>().erase(id);
	}

	/**
	 * @brief		Invokes all registered callbacks for a specific event type
	 * @tparam T	Event Type and data type that's passed into the event listeners
	 * @param data	The event data to pass to the listeners
	 * @warning		Doesn't support Subscribing/Unsubscribing during trigger
	 */
	template<typename T>
	static void Trigger(const T& data)
	{
		auto& listeners = GetListeners<T>();

		for (auto& [id, callback] : listeners)
			callback(data);
	}

	/**
	 * @brief		Removes all subscribers for a specific event type
	 * @tparam T	The event identifier type to clear 
	 */
	template<typename T>
	static void Clear()
	{
		GetListeners<T>().clear();
	}

private:
	template<typename T>
	static std::unordered_map<EventId, std::function<void(const T&)>>& GetListeners()
	{
		static std::unordered_map<EventId, std::function<void(const T&)>> listeners;
		return listeners;
	}

	inline static EventId nextId = 0;
};
