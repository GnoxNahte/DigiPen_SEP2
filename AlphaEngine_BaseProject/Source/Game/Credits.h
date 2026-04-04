/*!
@file	Credits.h
@author	Ethan Ong
@brief	Declares Credits class, for use in the main menu credits section
		All the credits are stored in a json file, making it easy to add new credits

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
#pragma once
#include <AEEngine.h>
#include <string>
#include <vector>
#include <functional>

#include "../Utils/FileHelper.h"
#include "../Editor/EditorUtils.h"

class Credits : Inspectable
{
public:
	Credits(std::function<void()> onExit);
	~Credits();

	void Reset();
	void Update();
	void Render();

	// Start playing credits. 
	// Will call onExit when it's done
	void StartCredits();

private:
	struct Config
	{
		// Not from JSON
		s8 fontId = -1;
		
		// ===== From JSON =====
		// === Changes as the game runs ===
		float transparency = -1.f;

		// === Won't change after reading from json ===
		// NOTE: Key is fadeTime.
		// fadeSpeed = 1.f / fadeTime
		float fadeSpeed = 1.f;
		float scrollSpeed = 1.f;

		// Bottom spacing / CSS bottom margin
		float sectionSpacing = 0.2f;
		float titleSpacing = 0.1f;
		float namesSpacing = 0.1f;

		// Horizontal spacing between columns
		float columnSpacing = 0.05f;
	};

	struct Text
	{
		Text() = default;
		Text(const std::string& str, s8 fontId); // Calls SetText

		bool HasText();
		void SetText(const std::string& str, s8 fontId);
		void PrintCenter(s8 fontId, float x, float y, float r, float g, float b, float a);

		std::string text{};
		AEVec2 size{};
	};

	struct CreditsData
	{
		CreditsData(const rapidjson::GenericObject<false, rapidjson::Value>& obj, const Config& config);

		// Return new yOffset (offset + height)
		float Print(float yOffset, float xOffset = 0);

		Text title;
		std::vector<Text> names; // Names/Libraries
		// Note:
		// - Only supports 2 columns for now
		// - While it technically allows nesting columns, it isn't supported.
		std::vector<CreditsData> columns; 

		AEVec2 size{};
		const Config& config;

		static constexpr const char* titleKey = "title";
		static constexpr const char* namesKey = "names";
		static constexpr const char* columnsKey = "columns";
	};

	AEGfxVertexList* mesh;

	Config config;
	std::vector<CreditsData> data;
	static constexpr const char* dataKey = "data";

	float animateOffset = 0.f;
	float totalHeight = -1.f;

	std::function<void()> onExit;

	// Inherited via Inspectable
	void DrawInspector() override;
};
