#include <fstream>
#include <iostream>
#include <string>
#include <rapidjson/document.h>

#include "PlayerStats.h"
#include "../../Utils/FileHelper.h"

namespace
{
	// doc - can be rapidjson::Document or rapidjson::GenericObject
	void LoadBox(const rapidjson::GenericObject<false, rapidjson::Value>& obj, Box& box)
	{
		auto positionObj = obj["position"].GetObj();
		box.position.x = positionObj["x"].GetFloat();
		box.position.y = positionObj["y"].GetFloat();

		auto sizeObj = obj["size"].GetObj();
		box.size.x = sizeObj["x"].GetFloat();
		box.size.y = sizeObj["y"].GetFloat();
	}

	void LoadAttack(const rapidjson::GenericObject<false, rapidjson::Value>& obj, AttackStats& attack)
	{
		attack.damage = obj["damage"].GetInt();
		attack.knockbackForce = obj["knockbackForce"].GetFloat();
		
		LoadBox(obj["collider"].GetObj(), attack.collider);
	}
}

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
	attackMaxSpeedMultiplier = doc["attackMaxSpeedMultiplier"].GetFloat();

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

	LoadBox(doc["groundChecker"].GetObj(), groundChecker);
	LoadBox(doc["ceilingChecker"].GetObj(), ceilingChecker);
	LoadBox(doc["leftWallChecker"].GetObj(), leftWallChecker);
	LoadBox(doc["rightWallChecker"].GetObj(), rightWallChecker);

	auto playerHeightObj = doc["playerSize"].GetObj();
	playerSize.x = playerHeightObj["x"].GetFloat();
	playerSize.y = playerHeightObj["y"].GetFloat();

	// === Combat stats ===
	maxHealth = doc["maxHealth"].GetInt();
	attackBuffer = doc["attackBuffer"].GetFloat();

	auto groundAttackArr = doc["groundAttacks"].GetArray();
	for (int i = 0; i < groundAttacks.size(); i++)
		LoadAttack(groundAttackArr[i].GetObj(), groundAttacks[i]);
	
	auto airAttacksArr = doc["airAttacks"].GetArray();
	for (int i = 0; i < airAttacks.size(); i++)
		LoadAttack(airAttacksArr[i].GetObj(), airAttacks[i]);

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
