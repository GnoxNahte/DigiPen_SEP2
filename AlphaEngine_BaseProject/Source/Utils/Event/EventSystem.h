#pragma once

#include <array>
#include <functional>
#include "Events.h"

using EventId = unsigned long;
using EventCallback = std::function<void(const BaseEventData&)>;

class EventSystem
{
public:
	static EventId Subscribe(EventType type, EventCallback callback);
	static void Unsubscribe(EventType type, EventId id);
	static void Trigger(const BaseEventData&);
	static void Trigger(EventType type);

	static void Clear();

private:
	using EventMap = std::unordered_map<EventId, EventCallback>;

	static EventId nextId;
	static std::array<EventMap, EventType::EVENT_COUNT> events;
};

