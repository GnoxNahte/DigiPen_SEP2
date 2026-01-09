#include <fstream>
#include <iostream>
#include <string>
#include <rapidjson/document.h>

#include "PlayerStats.h"
#include "../../Utils/FileHelper.h"

PlayerStats::PlayerStats(std::string file) : file(file)
{
	LoadFileData();
}

void PlayerStats::WatchFile()
{
#if _DEBUG
	std::cout << "watching file: " << file << std::endl;
#endif
}

void PlayerStats::LoadFileData()
{
	rapidjson::Document doc;
	bool success = FileHelper::TryReadJsonFile(file, doc);

	if (!success)
	{
		// @todo - Ethan: Handle error?
		return;
	}

	// ===== Convert JSON to class initialisation =====
	maxSpeed = doc["maxSpeed"].GetFloat();
	airStrafeMaxSpeed = doc["airStrafeMaxSpeed"].GetFloat();

	maxSpeedTime = doc["maxSpeedTime"].GetFloat();
	stopTime = doc["stopTime"].GetFloat();
	turnTime = doc["turnTime"].GetFloat();
	inAirTurnTime = doc["inAirTurnTime"].GetFloat();

	dashCooldown = doc["dashCooldown"].GetFloat();
	dashTime = doc["dashTime"].GetFloat();

	maxFallVelocity = doc["maxFallVelocity"].GetFloat();
	wallSlideMaxSpeedTime = doc["wallSlideMaxSpeedTime"].GetFloat();

	wallSlideMaxSpeed = doc["wallSlideMaxSpeed"].GetFloat();

	maxJumpHeight = doc["maxJumpHeight"].GetFloat();
	minJumpHeight = doc["minJumpHeight"].GetFloat();
	timeToMaxHeight = doc["timeToMaxHeight"].GetFloat();
	timeToGround = doc["timeToGround"].GetFloat();
	gravityMultiplierWhenRelease = doc["gravityMultiplierWhenRelease"].GetFloat();
	coyoteTime = doc["coyoteTime"].GetFloat();
	jumpBuffer = doc["jumpBuffer"].GetFloat();
	wallJumpHorizontalVelocity = doc["wallJumpHorizontalVelocity"].GetFloat();
	wallJumpHorizontalVelocityTowardsWall = doc["wallJumpHorizontalVelocityTowardsWall"].GetFloat();

	// ===== Pre-calculate other variables =====
	moveAcceleration = maxSpeed / maxSpeedTime;
	stopAcceleration = -maxSpeed / stopTime;
	turnAcceleration = 2.f * -maxSpeed / turnTime;
	inAirTurnAcceleration = 2.f * -airStrafeMaxSpeed / inAirTurnTime;

	gravity = (-2 * maxJumpHeight) / (timeToMaxHeight * timeToMaxHeight);
	fallingGravity = (-2 * maxJumpHeight) / (timeToGround * timeToGround);

	wallSlideAcceleration = wallSlideMaxSpeed / wallSlideMaxSpeedTime;

	jumpVelocity = (2.f * maxJumpHeight) / timeToMaxHeight;
	minJumpTime = 2.f * minJumpHeight / jumpVelocity;
}
