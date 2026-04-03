#pragma once
#include <AEEngine.h>
#include <string>
#include <vector>
#include "../Utils/FileHelper.h"
#include "../Editor/EditorUtils.h"

class Credits : Inspectable
{
public:
	Credits();
	~Credits();

	void Reset();
	void Update();
	void Render();

private:
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
		CreditsData(const rapidjson::GenericObject<false, rapidjson::Value>& obj, s8 fontId);

		// Return new yOffset (offset + height)
		float Print(s8 fontId, float yOffset, float xOffset = 0);

		Text title;
		std::vector<Text> names; // Names/Libraries
		// Note:
		// - Only supports 2 columns for now
		// - While it technically allows nesting columns, it isn't supported.
		std::vector<CreditsData> columns; 

		float width = -1.f;

		// Bottom spacing / CSS bottom margin
		inline static float sectionSpacing = 0.4f;
		inline static float titleSpacing = 0.1f;
		inline static float namesSpacing = 0.1f;
		// Horizontal spacing between columns
		inline static float columnSpacing = 0.05f; 

		static constexpr const char* titleKey = "title";
		static constexpr const char* namesKey = "names";
		static constexpr const char* columnsKey = "columns";
	};

	s8 fontId = -1;
	AEGfxVertexList* mesh;

	std::vector<CreditsData> data;
	static constexpr const char* dataKey = "data";

	float animateOffset = 0.f;

	// Inherited via Inspectable
	void DrawInspector() override;
};
