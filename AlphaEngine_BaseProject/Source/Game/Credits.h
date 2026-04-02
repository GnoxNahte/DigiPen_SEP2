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
	struct BaseCreditsData
	{
		virtual ~BaseCreditsData() = default;

		// Return offset + height
		virtual float Print(s8 fontId, float offset) = 0;
		virtual BaseCreditsData* Load(const rapidjson::GenericObject<false, rapidjson::Value>& obj) = 0;
	};

	// Single column
	struct SingleCreditData : public BaseCreditsData
	{
		std::string title;
		std::vector<std::string> names; // Names/Libraries

		// Keys for rapidjson
		static constexpr const char* titleKey = "title";
		static constexpr const char* namesKey = "names";

		virtual float Print(s8 fontId, float offset) override;
		virtual BaseCreditsData* Load(const rapidjson::GenericObject<false, rapidjson::Value>& obj);
	};

	// Two column
	struct DoubleCreditData : public BaseCreditsData
	{
		std::string title;
		SingleCreditData columns[2];

		// Keys for rapidjson
		static constexpr const char* titleKey = "title";
		static constexpr const char* columnsKey = "columns";

		virtual float Print(s8 fontId, float offset) override;
		virtual BaseCreditsData* Load(const rapidjson::GenericObject<false, rapidjson::Value>& obj);
	};

	inline static float spacing = 0.2f; 

	s8 fontId = -1;
	AEGfxVertexList* mesh;
	std::vector<BaseCreditsData*> data;

	static constexpr const char* dataKey = "data";

	float animateOffset = 0.f;

	// Inherited via Inspectable
	void DrawInspector() override;
};
