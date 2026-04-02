#include "Credits.h"
#include "../Utils/FileHelper.h"

Credits::Credits()
{
	fontId = AEGfxCreateFont("Assets/Pixellari.ttf", 36);

	rapidjson::Document doc;
	bool success = FileHelper::TryReadJsonFile("Assets/config/credits.json", doc);
	if (!success)
		return;
	
	rapidjson::GenericArray arr = doc["data"].GetArray();
	rapidjson::SizeType sz = arr.Size();
	data.reserve(sz);
	
	for (rapidjson::SizeType i = 0; i < sz; ++i)
	{
		rapidjson::GenericObject obj = arr[i].GetObj();

		// Two columns
		if (obj.HasMember(DoubleCreditData::columnsKey))
		{
			DoubleCreditData* d = new DoubleCreditData();
			d->Load(obj);
			data.push_back(d);
		}
		// Single column
		else
		{
			SingleCreditData* d = new SingleCreditData();
			d->Load(obj);
			data.push_back(d);
		}
	}

	(void)success;
}

Credits::~Credits()
{
	for (BaseCreditsData* i : data)
		delete i;
}

void Credits::Draw()
{
}

float Credits::SingleCreditData::Print(float )
{
	return 0.f;
}

Credits::BaseCreditsData* Credits::SingleCreditData::Load(const rapidjson::GenericObject<false, rapidjson::Value>& obj)
{
	title = obj[titleKey].GetString();

	// If only have title
	if (!obj.HasMember(namesKey))
		return this;

	auto& val = obj[namesKey];
	if (val.IsArray())
	{
		auto namesArr = val.GetArray();
		names.reserve(namesArr.Size());

		for (rapidjson::SizeType j = 0; j < namesArr.Size(); j++)
			names.emplace_back(namesArr[j].GetString());
	}
	else 
	{
		names.emplace_back(val.GetString());
	}

	return this;
}

float Credits::DoubleCreditData::Print(float )
{
	return 0.f;
}

Credits::BaseCreditsData* Credits::DoubleCreditData::Load(const rapidjson::GenericObject<false, rapidjson::Value>& obj)
{
	if (obj.HasMember(titleKey))
		title = obj[titleKey].GetString();
	else
		title = "";

	auto arr = obj[columnsKey].GetArray();
	assert(arr.Size() == 2);

	columns[0].Load(arr[0].GetObj());
	columns[1].Load(arr[1].GetObj());

	return nullptr;
}
