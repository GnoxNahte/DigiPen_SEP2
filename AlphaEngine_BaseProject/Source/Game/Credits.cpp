#include "Credits.h"

#include <imgui.h>

#include "Time.h"
#include "Camera.h"
#include "../Utils/FileHelper.h"
#include "../Utils/MeshGenerator.h"
#include "../Editor/Editor.h"
#include <iostream>

namespace
{
	// Print text center aligned
	// Returns text size
	float PrintTextCenter(s8 fontId, const char* str, float x, float y, float r, float g, float b, float a)
	{
		AEGfxPrint(fontId, str, x, y, 1.f, r, g, b, a);
	}
}

Credits::Credits() : Inspectable(true)
{
	fontId = AEGfxCreateFont("Assets/Pixellari.ttf", 36);
	mesh = MeshGenerator::GetRectMesh(
		static_cast<float>(AEGfxGetWindowWidth()),
		static_cast<float>(AEGfxGetWindowHeight()), 
		0xFF000000
	);

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

	Editor::RegisterSystem("Credits", this);
}

Credits::~Credits()
{
	AEGfxDestroyFont(fontId);
	AEGfxMeshFree(mesh);

	for (BaseCreditsData* i : data)
		delete i;

	Editor::UnregisterSystem("Credits", this);
}

void Credits::Reset()
{
	animateOffset = 0.f;
}

void Credits::Update()
{
	// Pause
	if (AEInputCheckCurr(AEVK_SPACE))
		return;

	if (AEInputCheckCurr(AEVK_LBUTTON))
	{
		s32 x, y;
		AEInputGetCursorPositionDelta(&x, &y);
		animateOffset -= static_cast<float>(y) / AEGfxGetWindowHeight();
	}
	else
		animateOffset += 0.1f * static_cast<float>(Time::GetInstance().GetDeltaTime());
}

void Credits::Render()
{
	AEMtx33 transform;
	AEMtx33Identity(&transform);
	AEMtx33Trans(&transform,
		Camera::position.x * Camera::scale,
		Camera::position.y * Camera::scale);

	AEGfxSetRenderMode(AE_GFX_RM_COLOR);

	AEGfxSetTransform(transform.m);
	AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);

	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);

	float offset = animateOffset;
	for (auto& i : data)
		offset = i->Print(fontId, offset);
}

void Credits::DrawInspector()
{
	ImGui::Begin("Credits", &isInspectorOpen);

	ImGui::DragFloat("Animate Offset", &animateOffset, 0.1f);

	ImGui::End();
}

float Credits::SingleCreditData::Print(s8 fontId, float offset)
{
	// Don't render if out of screen
	if (offset < -1.f - spacing || offset > 1.f)
		return offset - spacing;

	AEGfxPrint(fontId, title.c_str(), 0.f, offset, 1.f, 1.f, 1.f, 1.f, 1.f);
	return offset - spacing;
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

float Credits::DoubleCreditData::Print(s8 , float offset)
{
	return offset;
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
