#include <iostream>
#include <string>
#include <rapidjson/document.h>
#include <filesystem>
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

    void SaveBox(rapidjson::Value& obj, const Box& box, rapidjson::Document::AllocatorType& allocator)
    {
        obj.SetObject();

        rapidjson::Value position(rapidjson::kObjectType);
        position.AddMember("x", box.position.x, allocator);
        position.AddMember("y", box.position.y, allocator);
        obj.AddMember("position", position, allocator);

        rapidjson::Value size(rapidjson::kObjectType);
        size.AddMember("x", box.size.x, allocator);
        size.AddMember("y", box.size.y, allocator);
        obj.AddMember("size", size, allocator);
    }

    void SaveAttack(rapidjson::Value& obj, const AttackStats& attack, rapidjson::Document::AllocatorType& allocator)
    {
        obj.SetObject();
        obj.AddMember("damage", attack.damage, allocator);
        obj.AddMember("knockbackForce", attack.knockbackForce, allocator);

        rapidjson::Value collider(rapidjson::kObjectType);
        SaveBox(collider, attack.collider, allocator);
        obj.AddMember("collider", collider, allocator);
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

	dashSpeed = doc["dashSpeed"].GetFloat();
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

void PlayerStats::SaveFileData()
{
    rapidjson::Document doc;
    doc.SetObject();
    auto& allocator = doc.GetAllocator();

    // Movement Stats
    doc.AddMember("maxSpeed", maxSpeed, allocator);
    doc.AddMember("airStrafeMaxSpeed", airStrafeMaxSpeed, allocator);
    doc.AddMember("attackMaxSpeedMultiplier", attackMaxSpeedMultiplier, allocator);
    doc.AddMember("maxSpeedTime", maxSpeedTime, allocator);
    doc.AddMember("stopTime", stopTime, allocator);
    doc.AddMember("turnTime", turnTime, allocator);
    doc.AddMember("inAirTurnTime", inAirTurnTime, allocator);

    // Dash / Gravity
    doc.AddMember("dashSpeed", dashSpeed, allocator);
    doc.AddMember("dashCooldown", dashCooldown, allocator);
    doc.AddMember("dashTime", dashTime, allocator);
    doc.AddMember("maxFallVelocity", maxFallVelocity, allocator);
    doc.AddMember("wallSlideMaxSpeedTime", wallSlideMaxSpeedTime, allocator);
    doc.AddMember("wallSlideMaxSpeed", wallSlideMaxSpeed, allocator);

    // Jump Stats
    doc.AddMember("maxJumpHeight", maxJumpHeight, allocator);
    doc.AddMember("minJumpHeight", minJumpHeight, allocator);
    doc.AddMember("timeToMaxHeight", timeToMaxHeight, allocator);
    doc.AddMember("timeToGround", timeToGround, allocator);
    doc.AddMember("gravityMultiplierWhenRelease", gravityMultiplierWhenRelease, allocator);
    doc.AddMember("coyoteTime", coyoteTime, allocator);
    doc.AddMember("jumpBuffer", jumpBuffer, allocator);
    doc.AddMember("wallJumpHorizontalVelocity", wallJumpHorizontalVelocity, allocator);
    doc.AddMember("wallJumpHorizontalVelocityTowardsWall", wallJumpHorizontalVelocityTowardsWall, allocator);

    // Box Checkers
    rapidjson::Value groundObj(rapidjson::kObjectType);
    SaveBox(groundObj, groundChecker, allocator);
    doc.AddMember("groundChecker", groundObj, allocator);

    rapidjson::Value ceilingObj(rapidjson::kObjectType);
    SaveBox(ceilingObj, ceilingChecker, allocator);
    doc.AddMember("ceilingChecker", ceilingObj, allocator);

    rapidjson::Value leftWallObj(rapidjson::kObjectType);
    SaveBox(leftWallObj, leftWallChecker, allocator);
    doc.AddMember("leftWallChecker", leftWallObj, allocator);

    rapidjson::Value rightWallObj(rapidjson::kObjectType);
    SaveBox(rightWallObj, rightWallChecker, allocator);
    doc.AddMember("rightWallChecker", rightWallObj, allocator);

    // Player Size
    rapidjson::Value playerSizeObj(rapidjson::kObjectType);
    playerSizeObj.AddMember("x", playerSize.x, allocator);
    playerSizeObj.AddMember("y", playerSize.y, allocator);
    doc.AddMember("playerSize", playerSizeObj, allocator);

    // Combat
    doc.AddMember("maxHealth", maxHealth, allocator);
    doc.AddMember("attackBuffer", attackBuffer, allocator);

    // Attack Arrays
    rapidjson::Value groundAttacksArr(rapidjson::kArrayType);
    for (const auto& attack : groundAttacks)
    {
        rapidjson::Value attackVal(rapidjson::kObjectType);
        SaveAttack(attackVal, attack, allocator);
        groundAttacksArr.PushBack(attackVal, allocator);
    }
    doc.AddMember("groundAttacks", groundAttacksArr, allocator);

    rapidjson::Value airAttacksArr(rapidjson::kArrayType);
    for (const auto& attack : airAttacks)
    {
        rapidjson::Value attackVal(rapidjson::kObjectType);
        SaveAttack(attackVal, attack, allocator);
        airAttacksArr.PushBack(attackVal, allocator);
    }
    doc.AddMember("airAttacks", airAttacksArr, allocator);

    std::string actualAssetPath = "../../" + file;
    if (std::filesystem::exists(actualAssetPath))
        FileHelper::TryWriteJsonFile(actualAssetPath, doc);
    
    FileHelper::TryWriteJsonFile(file, doc);
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
        ifChanged = ImGui::DragFloat("Dash Speed", &dashSpeed, 0.01f) || ifChanged;
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

    if (ImGui::Button("Reset to File"))
        LoadFileData();

    ImGui::SameLine();
    if (ImGui::Button("Save"))
        SaveFileData();

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
