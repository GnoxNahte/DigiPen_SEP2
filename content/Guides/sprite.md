## Sprite usage {#sprite_usage}

### Description
A class to help with rendering animations with basic animation state

@note This is for sprites with animation. If you don't need animation, use [AlphaEngine's texture](https://distance3.sg.digipen.edu/2026sg-spring/pluginfile.php/2647/mod_resource/content/13/site/rendering_sprites.html) or [Spritesheet](https://distance3.sg.digipen.edu/2026sg-spring/pluginfile.php/2647/mod_resource/content/13/site/spritesheets.html)  instead.

### Setup - Importing assets
1. Import your sprite into `Assets` folder
2. Create a text file with the same name as the sprite asset (including the extension) but add `.meta` to the end of the file. <br>For example, if file name is `player.png`, add a `player.png.meta` text file.

Reference: 
- [Assets/Art/rvos/Adventurer.png](https://github.com/GnoxNahte/DigiPen_SEP2/blob/main/Assets/Art/rvros/Adventurer.png)
- [Assets/Art/rvos/Adventurer.png.meta](https://github.com/GnoxNahte/DigiPen_SEP2/blob/main/Assets/Art/rvros/Adventurer.png.meta)

### Setup - .meta file {#meta-file-setup}
See SpriteMetadata for what data to put inside. 

I'll try to update the docs but it might be outdated. Best to see the SpriteMetadata file directly to see what needs to be put inside.

Example JSON:
```json
{
	"stateInfo": [
		{ "name": "IDLE", "frameCount": 4 },
		{ "name": "RUN",  "frameCount": 2, "sampleRate": 24 }
	],
	"defaultSampleRate": 12,
	"pivot": {
		"x": 0.5,
		"y": 0.4
	}
}
```

Explaining the params:
- `stateInfo` - Array of objects. Make sure to use `[]` instead of `{}`
	-  `name` - not used, just for reference/organising
	- `frameCount` - number of frames in the animation
	- `sampleRate` (optional) - will `defaultSampleRate` by default. Frame rate of animation
- `pivot` - Not used by Sprite directly but useful when setting the position during rendering
- `defaultSampleRate` - Default frame per second

@note
Sprite calculates the row and column and uses that to divide the sprite into individual frames
- `row` count = length of `stateInfo` 
- `col` count = loop through `stateInfo` and find the max `frameCount`

See [Assets/Art/rvos/Adventurer.png.meta](https://github.com/GnoxNahte/DigiPen_SEP2/blob/main/Assets/Art/rvros/Adventurer.png.meta) for another reference
### Setup - Using in code
1. In `.h`, include the Sprite file (location at `Utils/Sprite.h`)
2. In the class, 
	1. In header file, add a Sprite variable 
	2. In the constructor, pass in the path to the sprite file (including extension). <br> For example, "Assets/Art/player.png"

Example:
```cpp
// In Player.h
#include "Utils/Sprite.h"
class Player {
	Player();
	Sprite sprite;
}

// In Player.cpp
// Calls sprite constructor directly and passes the path as a parameter
Player::Player() : sprite("Assets/Player.png") 
{
	// Init player
}
```

Then, 
- Call Sprite.Update() to update the animation
- Call Sprite.Render() to render to screen

@warning
Render does NOT set the transform as it doesn't store any position, scale or rotation.<br>Still need to call `AEGfxSetTransform`.<br>Remember to multiply Camera::scale!

Reference (After clicking this link, click "Go to source" or see the source in visual studio):
- Player.h 
- Player.cpp
### Changing animation state
Call Sprite.SetState to change state. This will the select the row specified in @ref meta-file-setup

If you want to only change  until the current animation reaches the end, set the 2nd parameter to true.<br>For example, switching from attack (wait for attack anim to finish) to idle. <br>This is only visual though. Make sure the actual game shows the correct result.

Recommended to use a Enum for easier readability. Example: Player::AnimState

```cpp
sprite.SetState(0); // Plays animation for 1st row
sprite.SetState(3); // Plays animation for 4th row
sprite.SetState(5, true); // Only starts to play the animation for the 6th row when the current animation finishes (reaches the end of the row)
```

