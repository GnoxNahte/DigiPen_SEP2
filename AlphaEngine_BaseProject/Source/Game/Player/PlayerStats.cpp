#include <fstream>
#include <iostream>
#include <string>
#include <rapidjson/document.h>
#include <imgui.h>

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

	CalculateDerivedVariables();
}

void PlayerStats::OnDataChanged()
{
	CalculateDerivedVariables();

	// @todo save to file
}

void PlayerStats::DrawInspector()
{
    bool ifChanged = false;

    ImGui::PushItemWidth(150);
    // ===== Horizontal Movement =====
    if (ImGui::TreeNode("Horizontal Movement"))
    {
        ifChanged = ImGui::DragFloat("Max Speed", &maxSpeed, 0.1f) || ifChanged;
        ifChanged = ImGui::DragFloat("Air Strafe Max Speed", &airStrafeMaxSpeed, 0.1f) || ifChanged;
        ifChanged = ImGui::DragFloat("Attack Max Speed Multiplier", &attackMaxSpeedMultiplier, 0.01f) || ifChanged;
        ifChanged = ImGui::DragFloat("Max Speed Time", &maxSpeedTime, 0.01f) || ifChanged;
        ifChanged = ImGui::DragFloat("Stop Time", &stopTime, 0.01f) || ifChanged;
        ifChanged = ImGui::DragFloat("Turn Time", &turnTime, 0.01f) || ifChanged;
        ifChanged = ImGui::DragFloat("In Air Turn Time", &inAirTurnTime, 0.01f) || ifChanged;
        ifChanged = ImGui::DragFloat("Dash Cooldown", &dashCooldown, 0.01f) || ifChanged;
        ifChanged = ImGui::DragFloat("Dash Time", &dashTime, 0.01f) || ifChanged;

        ImGui::SeparatorText("Derived Stats");
        ImGui::BeginDisabled();
        ifChanged = ImGui::DragFloat("Move Acceleration", &moveAcceleration, 0.1f) || ifChanged;
        ifChanged = ImGui::DragFloat("Stop Acceleration", &stopAcceleration, 0.1f) || ifChanged;
        ifChanged = ImGui::DragFloat("Turn Acceleration", &turnAcceleration, 0.1f) || ifChanged;
        ifChanged = ImGui::DragFloat("In Air Turn Acceleration", &inAirTurnAcceleration, 0.1f) || ifChanged;
        ImGui::EndDisabled();

        ImGui::TreePop();
    }

    // ===== Vertical Movement =====
    if (ImGui::TreeNode("Vertical Movement"))
    {
        ifChanged = ImGui::DragFloat("Max Fall Velocity", &maxFallVelocity, 0.1f) || ifChanged;
        ifChanged = ImGui::DragFloat("Wall Slide Max Speed Time", &wallSlideMaxSpeedTime, 0.01f) || ifChanged;
        ifChanged = ImGui::DragFloat("Wall Slide Max Speed", &wallSlideMaxSpeed, 0.1f) || ifChanged;

        ImGui::SeparatorText("Derived Stats");
        ImGui::BeginDisabled();
        ifChanged = ImGui::DragFloat("Gravity", &gravity, 0.1f) || ifChanged;
        ifChanged = ImGui::DragFloat("Falling Gravity", &fallingGravity, 0.1f) || ifChanged;
        ifChanged = ImGui::DragFloat("Wall Slide Acceleration", &wallSlideAcceleration, 0.1f) || ifChanged;
        ImGui::EndDisabled();

        ImGui::TreePop();
    }

    // ===== Jumping =====
    if (ImGui::TreeNode("Jumping"))
    {
        ifChanged = ImGui::DragFloat("Max Jump Height", &maxJumpHeight, 0.1f) || ifChanged;
        ifChanged = ImGui::DragFloat("Min Jump Height", &minJumpHeight, 0.1f) || ifChanged;
        ifChanged = ImGui::DragFloat("Time To Max Height", &timeToMaxHeight, 0.01f) || ifChanged;
        ifChanged = ImGui::DragFloat("Time To Ground", &timeToGround, 0.01f) || ifChanged;
        ifChanged = ImGui::DragFloat("Gravity Multiplier When Release", &gravityMultiplierWhenRelease, 0.01f) || ifChanged;
        ImGui::SetItemTooltip("When player releases 'Jump', gravity will be multiplied");

        ifChanged = ImGui::DragFloat("Coyote Time", &coyoteTime, 0.01f) || ifChanged;
        ImGui::SetItemTooltip("Allows player to jump for some time after leaving the platform");

        ifChanged = ImGui::DragFloat("Jump Buffer", &jumpBuffer, 0.01f) || ifChanged;
        ImGui::SetItemTooltip("Allows the player to jump if they press and release 'Jump' before reaching the ground");

        ifChanged = ImGui::DragFloat("Wall Jump Horizontal Velocity", &wallJumpHorizontalVelocity, 0.1f) || ifChanged;
        ifChanged = ImGui::DragFloat("Wall Jump Horizontal Velocity Towards Wall", &wallJumpHorizontalVelocityTowardsWall, 0.1f) || ifChanged;

        ImGui::SeparatorText("Derived Stats");
        ImGui::BeginDisabled();
        ifChanged = ImGui::DragFloat("Jump Velocity", &jumpVelocity, 0.1f) || ifChanged;
        ifChanged = ImGui::DragFloat("Min Jump Time", &minJumpTime, 0.01f) || ifChanged;
        ImGui::EndDisabled();

        ImGui::TreePop();
    }

    // ===== Colliders =====
    if (ImGui::TreeNode("Colliders"))
    {
        ImGui::PushItemWidth(200);

        ImGui::Text("Ground Checker");
        ifChanged = ImGui::DragFloat2("GroundPos", &groundChecker.position.x, 0.01f) || ifChanged;
        ifChanged = ImGui::DragFloat2("GroundSize", &groundChecker.size.x, 0.01f) || ifChanged;

        ImGui::Text("Ceiling Checker");
        ifChanged = ImGui::DragFloat2("CeilingPos", &ceilingChecker.position.x, 0.01f) || ifChanged;
        ifChanged = ImGui::DragFloat2("CeilingSize", &ceilingChecker.size.x, 0.01f) || ifChanged;

        ImGui::Text("Left Wall Checker");
        ifChanged = ImGui::DragFloat2("LeftWallPos", &leftWallChecker.position.x, 0.01f) || ifChanged;
        ifChanged = ImGui::DragFloat2("LeftWallSize", &leftWallChecker.size.x, 0.01f) || ifChanged;

        ImGui::Text("Right Wall Checker");
        ifChanged = ImGui::DragFloat2("RightWallPos", &rightWallChecker.position.x, 0.01f) || ifChanged;
        ifChanged = ImGui::DragFloat2("RightWallSize", &rightWallChecker.size.x, 0.01f) || ifChanged;

        ImGui::PopItemWidth();
        ImGui::TreePop();
    }

    // ===== Combat =====
    if (ImGui::TreeNode("Combat"))
    {
        ifChanged = ImGui::DragInt("Max Health", &maxHealth, 1.0f, 1, 1000) || ifChanged;
        ifChanged = ImGui::DragFloat("Attack Buffer", &attackBuffer, 0.01f) || ifChanged;

        ImGui::TreePop();
    }

    // ===== Others =====
    if (ImGui::TreeNode("Others"))
    {
        ifChanged = ImGui::DragFloat2("Player Size", &playerSize.x, 0.01f) || ifChanged;

        ImGui::TreePop();
    }

    if (ifChanged)
        OnDataChanged();

    ImGui::PopItemWidth();
}

void PlayerStats::CalculateDerivedVariables()
{
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
