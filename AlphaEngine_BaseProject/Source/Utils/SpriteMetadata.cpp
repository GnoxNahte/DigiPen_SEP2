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
	static const char *mStateInfo = "stateInfo",
					  *mPivot = "pivot",
					  *mDefaultSampleRate = "defaultSampleRate";

	if (!document.HasMember(mStateInfo) || !document.HasMember(mPivot) || !document.HasMember(mDefaultSampleRate))
	{
		std::cout << "File (" << originalFile << ") missing metadata members." << std::endl;
		return;
	}

	this->defaultSampleRate = document[mDefaultSampleRate].GetInt();

	auto stateInfoArr = document[mStateInfo].GetArray();
	this->rows = stateInfoArr.Size();

	// Copy the rapidjson array result into framesPerRow vector
	this->stateInfoRows.reserve(rows);
	for (auto& stateInfoObj : stateInfoArr)
	{
		this->stateInfoRows.emplace_back(
			stateInfoObj["name"].GetString(),
			stateInfoObj["frameCount"].GetInt(),
			stateInfoObj.HasMember("sampleRate") ? stateInfoObj["sampleRate"].GetInt() : defaultSampleRate
		);
	}

	// Find the max frame count and assign it to cols
	for (auto& stateInfo : stateInfoRows)
	{
		if (stateInfo.frameCount > cols)
			cols = stateInfo.frameCount;
	}
	
	auto pivotObject = document[mPivot].GetObject();
	pivot.x = pivotObject["x"].GetFloat();
	pivot.y = pivotObject["y"].GetFloat();
}

SpriteMetadata::StateInfo::StateInfo(std::string name, int frameCount, int sampleRate) :
	name(name),
	frameCount(frameCount),
	sampleRate(sampleRate),
	timePerFrame(1.f / sampleRate)
{
}
