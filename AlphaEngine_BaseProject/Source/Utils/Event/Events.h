#pragma once
#include "../../Game/BuffCards.h"

enum EventType
{
	// === General ===
	OnPause,

	// === In Game ===
	OnBuffSelected,
	OnBuffRemoved,

	OnPlayerDeath, 

	EVENT_COUNT,
};

struct BaseEventData
{
	EventType type;
	BaseEventData(EventType type) : type(type) {}
	virtual ~BaseEventData() = default;
};

struct BuffSelectedEventData : public BaseEventData
{
	const BuffCard& card;

	BuffSelectedEventData(const BuffCard& card) : BaseEventData(EventType::OnBuffSelected), card(card) {}
};
