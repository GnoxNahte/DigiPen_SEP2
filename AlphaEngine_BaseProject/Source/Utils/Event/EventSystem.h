#pragma once

#include <array>
#include <functional>

using EventId = unsigned long;

class EventSystem
{
public:
	template<typename T>
	static EventId Subscribe(std::function<void(const T&)> callback)
	{
		EventId id = nextId++;
		auto& listeners = GetListeners<T>();
		listeners.emplace(id, callback);
		return id;
	}

	template<typename T>
	static void Unsubscribe(EventId id)
	{
		GetListeners<T>().erase(id);
	}

	template<typename T>
	static void Trigger(const T& data)
	{
		auto& listeners = GetListeners<T>();

		for (auto& [id, callback] : listeners)
			callback(data);
	}

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
