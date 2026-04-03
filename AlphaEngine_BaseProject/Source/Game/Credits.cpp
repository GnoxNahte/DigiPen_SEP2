#include "Credits.h"

#include <imgui.h>

#include "Time.h"
#include "Camera.h"
#include "../Utils/FileHelper.h"
#include "../Utils/MeshGenerator.h"
#include "../Utils/QuickGraphics.h"
#include "../Editor/Editor.h"
#include <iostream>

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
		data.emplace_back(arr[i].GetObj(), fontId);

	Reset();

	Editor::RegisterSystem("Credits", this);
}

Credits::~Credits()
{
	AEGfxDestroyFont(fontId);
	AEGfxMeshFree(mesh);

	Editor::UnregisterSystem("Credits", this);
}

void Credits::Reset()
{
	animateOffset = 0.f;
	// Debug
	animateOffset = 5.5f;
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
		offset = i.Print(fontId, offset);
}

void Credits::DrawInspector()
{
	ImGui::Begin("Credits", &isInspectorOpen);

	ImGui::DragFloat("Animate Offset", &animateOffset, 0.1f);
	ImGui::DragFloat("Section Spacing", &CreditsData::sectionSpacing, 0.01f);
	ImGui::DragFloat("Title Spacing", &CreditsData::titleSpacing, 0.01f);
	ImGui::DragFloat("Names Spacing", &CreditsData::namesSpacing, 0.01f);
	ImGui::DragFloat("Column Spacing", &CreditsData::columnSpacing, 0.01f);

	ImGui::End();
}

// ================
// | Credits Data |
// ================
Credits::CreditsData::CreditsData(const rapidjson::GenericObject<false, rapidjson::Value>& obj, s8 fontId)
{
	// Title
	if (obj.HasMember(titleKey))
		title.SetText(obj[titleKey].GetString(), fontId);
	else
		title.SetText("", fontId);

	// Names
	if (obj.HasMember(namesKey))
	{
		auto& val = obj[namesKey];
		if (val.IsArray())
		{
			auto namesArr = val.GetArray();
			names.reserve(namesArr.Size());

			for (rapidjson::SizeType j = 0; j < namesArr.Size(); j++)
				names.emplace_back(namesArr[j].GetString(), fontId);
		}
		else
		{
			names.emplace_back(val.GetString(), fontId);
		}
	}

	// Columns
	if (obj.HasMember(columnsKey))
	{
		auto arr = obj[columnsKey].GetArray();
		assert(arr.Size() == 2);
	
		columns.emplace_back(arr[0].GetObj(), fontId);
		columns.emplace_back(arr[1].GetObj(), fontId);
	}

	// ===== Find width (Max width of all text) =====
	width = title.size.x;
	
	for (auto& i : names)
		width = max(width, i.size.x);
	
	for (auto& i : columns)
		width = max(width, i.width);
}

float Credits::CreditsData::Print(s8 fontId, float yOffset, float xOffset)
{
	//// Don't render if out of screen
	//float height = titleSpacing * title.HasText() +  * (names.size() + title.HasText());
	//if (offset < -1.f - spacing || offset > 1.f + height)
	//	return offset - height;

	if (title.HasText())
	{
		title.PrintCenter(fontId, xOffset, yOffset, 1.f, 0.82f, 0.35f, 1.f);
		yOffset -= titleSpacing + title.size.y;
	}
	
	for (auto& i : names)
	{
		i.PrintCenter(fontId, xOffset, yOffset, 1.f, 1.f, 1.f, 1.f);
		yOffset -= namesSpacing + i.size.y;
	}

	if (!columns.empty())
	{
		float spacing = columnSpacing + max(columns[0].width, columns[1].width) * 0.5f;
		float offset0 = columns[0].Print(fontId, yOffset, xOffset - spacing);
		float offset1 = columns[1].Print(fontId, yOffset, xOffset + spacing);
		yOffset = max(offset0, offset1);
	}

	return yOffset - sectionSpacing;
}

// ================
// | Credits Text |
// ================
Credits::Text::Text(const std::string& str, s8 fontId)
{
	SetText(str, fontId);
}

bool Credits::Text::HasText()
{
	return !text.empty();
}

void Credits::Text::SetText(const std::string& str, s8 fontId)
{
	text = str;
	AEGfxGetPrintSize(fontId, str.c_str(), 1.f, &size.x, &size.y);
}

void Credits::Text::PrintCenter(s8 fontId, float x, float y, float r, float g, float b, float a)
{
	AEGfxPrint(fontId, text.c_str(), x - size.x * 0.5f, y, 1.f, r, g, b, a);
}
