#include <rapidjson/document.h>
#include <iostream>
#include <string>

#include "SpriteMetadata.h"
#include "FileHelper.h"

SpriteMetadata::SpriteMetadata(std::string originalFile)
{
	originalFile += ".meta";

	rapidjson::Document document;
	bool success = FileHelper::TryReadJsonFile(originalFile, document);
	if (!success)
	{
		// todo? - handle error
		return;
	}

	// JSON member names
	static const char *mframesPerRow = "framesPerRow",
					  *mframesPerSecond = "framesPerSecond",
					  *mPivot = "pivot";

	if (!document.HasMember(mframesPerRow) || !document.HasMember(mframesPerSecond)|| !document.HasMember(mPivot))
	{
		std::cout << "File (" << originalFile << ") missing metadata members." << std::endl;
		return;
	}

	auto framesPerRowArr = document[mframesPerRow].GetArray();

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

	auto pivotObject = document[mPivot].GetObject();
	pivot.x = pivotObject["x"].GetFloat();
	pivot.y = pivotObject["y"].GetFloat();
}

