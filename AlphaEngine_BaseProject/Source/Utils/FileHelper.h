#pragma once
#include <rapidjson/document.h>
#include <string>

/**
 * @brief Contains helper functions to make File IO easier
 */
class FileHelper
{
public:
	static bool TryReadJsonFile(std::string file, rapidjson::Document& doc);

private:
	// Disable creating an instance. Static class
	FileHelper() = delete;
};

