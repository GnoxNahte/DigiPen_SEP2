#pragma once
#include <AEEngine.h>
#include <string>
#include <vector>
#include "../Utils/FileHelper.h"

class Credits
{
public:
	Credits();
	~Credits();
	void Draw();

private:
	struct BaseCreditsData
	{
		virtual ~BaseCreditsData() = default;

		// Return offset + height
		virtual float Print(float offset) = 0;
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

		virtual float Print(float offset) override;
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

		virtual float Print(float offset) override;
		virtual BaseCreditsData* Load(const rapidjson::GenericObject<false, rapidjson::Value>& obj);
	};

	s8 fontId = -1;
	std::vector<BaseCreditsData*> data;
	static constexpr const char* dataKey = "data";
};
