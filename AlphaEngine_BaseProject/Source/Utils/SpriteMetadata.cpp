#include <rapidjson/document.h>
#include <iostream>
#include <string>

#include "SpriteMetadata.h"
#include "FileHelper.h"

SpriteMetadata::SpriteMetadata(std::string originalFile)
{
	originalFile += ".meta";
	rapidjson::Document document;
	bool readSuccess = FileHelper::TryReadJsonFile(originalFile, document);
	if (!readSuccess)
	{
		// todo? - handle error
		return;
	}

	// JSON member names
	static const char *mframesPerRow = "framesPerRow",
					  *mframesPerSecond = "framesPerSecond";

	if (!document.HasMember(mframesPerRow) || !document.HasMember(mframesPerSecond))
	{
		std::cout << "File (" << originalFile << ") missing metadata members." << std::endl;
		return;
	}

	rapidjson::GenericArray framesPerRowArr = document[mframesPerRow].GetArray();

	this->rows = framesPerRowArr.Size();
	this->cols = 0; // Will set later in for loop

	// Copy the rapidjson array result into framesPerRow vector
	this->framesPerRow.reserve(framesPerRowArr.Size());
	for (auto& i : framesPerRowArr)
	{
		int colCount = i.GetInt();
		this->framesPerRow.emplace_back(colCount);

		// Get max column count
		if (colCount > this->cols)
			this->cols = colCount;
	}
	
	this->framesPerSecond = document[mframesPerSecond].GetFloat();
}

