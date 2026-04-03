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
		s8 fontId = -1;
		float transparency = -1.f;

		float fadeSpeed = 1.f / 1.f;
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

		// Bottom spacing / CSS bottom margin
		inline static float sectionSpacing = 0.2f;
		inline static float titleSpacing = 0.1f;
		inline static float namesSpacing = 0.1f;
		// Horizontal spacing between columns
		inline static float columnSpacing = 0.05f; 

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
