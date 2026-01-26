#pragma once
#include <string>
#include <vector>
#include <AEVec2.h>

/**
 * @brief	Stores metadata for each sprite. 
			Metadata is stored in a related file with the original sprite.
			If the original sprite file name is "player.png", metadata should be in "player.png.meta"
 */
struct SpriteMetadata
{
	struct StateInfo
	{
		// === In JSON ===
		// Not using for anything, just for organising. Exclude in release?
		std::string name;

		/**
		 * @brief Number of frames each frame has
		 */
		int frameCount;

		/**
		 * @brief Frame per second / How fast to play the animation
		 * @note This is optional. Will use defaultSampleRate instead
		 */
		int sampleRate;

		// === NOT in JSON ===
		// Derived from sample rate
		float timePerFrame;

		StateInfo(std::string name, int frameCount, int sampleRate);
	};

	// === In JSON ===
	
	/**
	 * @brief Contains all the info per state.
	 */
	std::vector<StateInfo> stateInfoRows;

	/**
	 * @brief Pivot position for all frames. Range: 0..1
	 * @todo Shift to stateInfo or make it default like defaultSampleRate?
	 */
	AEVec2 pivot{ 0.5f, 0.5f };

	/**
	 * @brief Default frame per second / How fast to play the animation
	 *		  Will be used when the state doesn't specify it
	 */
	int defaultSampleRate;

	// === Derived from JSON data. NOT in JSON ===
	int rows = -1, cols = -1;

	SpriteMetadata(std::string file);
};

