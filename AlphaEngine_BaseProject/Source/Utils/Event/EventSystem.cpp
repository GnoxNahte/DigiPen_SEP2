#include "EventSystem.h"

EventId EventSystem::nextId = 0;
std::array<EventSystem::EventMap, EventType::EVENT_COUNT> EventSystem::events;

EventId EventSystem::Subscribe(EventType type, EventCallback callback)
{
	EventId id = nextId++;
	events[type].emplace(id, callback);
	return id;
}

void EventSystem::Unsubscribe(EventType type, EventId id)
{
	events[type].erase(id);
}

void EventSystem::Trigger(const BaseEventData& event)
{
	EventMap& currEvents = events[event.type];
	for (const auto& [id, callback] : currEvents)
		callback(event);
}

void EventSystem::Trigger(EventType type)
{
	Trigger(BaseEventData{ type });
}

void EventSystem::Clear()
{
	for (auto& i : events)
		i.clear();
}
