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
	// === In JSON ===
	
	/**
	 * @brief State the numbers in frames each row has. 
	 *		  In JSON, it should be a int[]
	 */
	std::vector<int> framesPerRow;

	/**
	 * @brief Frames per second or sample rate
	 */
	float framesPerSecond;

	/**
	 * @brief Pivot position for all frames. Range: 0..1
	 */
	AEVec2 pivot;

	// === Derived from JSON data. NOT in JSON ===
	int rows, cols;

	SpriteMetadata(std::string file);
};

