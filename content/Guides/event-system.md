# Event System {#event-system}
## Description
Event system that allows to 
- \ref EventSystem.Subscribe "Subscribe"
- \ref EventSystem.Unsubscribe "Unsubscribe"
- \ref EventSystem.Trigger "Events"

## Usage
Create a class that contains the data to be sent when an event is triggered.<br>If there's no data that's needed to be transferred, an empty class is good too. Just need an identification for the event.

### Subscribing and Unsubscribing to events
Steps:
1. In your class, add an `EventId` variable
2. Call `EventSystem::Subscribe<CustomEvent>` and store the return data to the `EventId` variable in step 1.
3. Call `EventSystem::Unsubscribe<CustomEvent>` when you don't need it. Pass in the `EventId` from step 1.

When there's a Subscribe there **must** be another Unsubscribe call. If not might cause crash or lots of wasted memory. Similar to memory leak.

Recommended to call Subscribe in constructor and Unsubscribe in Deconstructor.

### Triggering events

Call EventSystem::Trigger, pass the custom event as a template parameter.

See the example below for different ways on calling it.

## Example

```cpp
struct PlayerDeathEvent
{
	const Player& player;
};

class Player
{
public:
	int health;
	void TakeDamage(int damage)
	{
		// These 3 lines do the same thing, just showing different variations
		EventSystem::Trigger<PlayerDeathEvent>({ *this });
		EventSystem::Trigger(PlayerDeathEvent{ *this });
		EventSystem::Trigger(PlayerDeathEvent{ .player = *this });
	}
};

class SampleClass
{
	void OnPlayerDied(const PlayerDeathEvent& ev)
	{
		std::cout << "Player died! - health:" << ev.player.health << "\n";
	}

	// ID used to Unsubscribe later
	EventId playerDeathEventId;

	SampleClass()
	{
		playerDeathEventId = EventSystem::Subscribe<PlayerDeathEvent>([this](PlayerDeathEvent ev) {
			OnPlayerDied(ev);
		});
	}

	~SampleClass()
	{
		EventSystem::Unsubscribe<PlayerDeathEvent>(playerDeathEventId);
	}
};
```

Another example that's in the code: Player applying buffs

