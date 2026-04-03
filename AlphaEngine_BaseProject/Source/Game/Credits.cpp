#include "Credits.h"

#include <imgui.h>

#include "Time.h"
#include "Camera.h"
#include "../Utils/FileHelper.h"
#include "../Utils/MeshGenerator.h"
#include "../Utils/QuickGraphics.h"
#include "../Editor/Editor.h"
#include <iostream>
#include <numeric>

Credits::Credits(std::function<void()> onExit) :
	Inspectable(true), 
	onExit(onExit)
{
	mesh = MeshGenerator::GetRectMesh(
		static_cast<float>(AEGfxGetWindowWidth()),
		static_cast<float>(AEGfxGetWindowHeight()), 
		0xFF000000
	);

	rapidjson::Document doc;
	bool success = FileHelper::TryReadJsonFile("Assets/config/credits.json", doc);
	if (!success)
		return;

	// ===== Set config =====
	config.fontId = AEGfxCreateFont("Assets/Pixellari.ttf", 36);
	config.transparency = -1.f;

	// === Config JSON data ===
	auto configObj = doc["config"].GetObj();
	config.fadeSpeed = 1.f / configObj["fadeTime"].GetFloat();
	config.scrollSpeed = configObj["scrollSpeed"].GetFloat();

	config.sectionSpacing = configObj["sectionSpacing"].GetFloat();
	config.titleSpacing = configObj["titleSpacing"].GetFloat();
	config.namesSpacing = configObj["namesSpacing"].GetFloat();
	
	config.columnSpacing = configObj["columnSpacing"].GetFloat();

	// ===== Read credits data =====
	rapidjson::GenericArray arr = doc["data"].GetArray();
	rapidjson::SizeType sz = arr.Size();
	data.reserve(sz);
	
	for (rapidjson::SizeType i = 0; i < sz; ++i)
		data.emplace_back(arr[i].GetObj(), config);

	totalHeight = 0.f;
	for (auto& i : data)
		totalHeight += i.size.y;

	Reset();

	Editor::RegisterSystem("Credits", this);
}

Credits::~Credits()
{
	AEGfxDestroyFont(config.fontId);
	AEGfxMeshFree(mesh);

	Editor::UnregisterSystem("Credits", this);
}

void Credits::Reset()
{
	animateOffset = -1.f;
}

void Credits::Update()
{
	if (config.transparency < 0.f)
		return;

	float dt = static_cast<float>(Time::GetInstance().GetDeltaTime());

	// Skip to end
	if (AEInputCheckCurr(AEVK_ESCAPE))
		animateOffset = totalHeight + 10; 

	// Pause
	if (AEInputCheckCurr(AEVK_SPACE))
		return;

	if (AEInputCheckCurr(AEVK_LBUTTON))
	{
		s32 x, y;
		AEInputGetCursorPositionDelta(&x, &y);
		animateOffset -= static_cast<float>(y * 2) / AEGfxGetWindowHeight();
	}
	else
		animateOffset += config.scrollSpeed * dt;

	if (animateOffset > totalHeight)
	{
		config.transparency -= config.fadeSpeed * dt;
		if (config.transparency < 0.f)
		{
			config.transparency = -1.f;
			onExit();
		}
	}
	else
		config.transparency = min(config.transparency + config.fadeSpeed * dt, 1.f);
}

void Credits::Render()
{
	if (config.transparency < 0.f)
		return;

	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	AEGfxSetTransparency(config.transparency);

	AEMtx33 transform;
	AEMtx33Identity(&transform);
	AEMtx33Trans(&transform,
		Camera::position.x * Camera::scale,
		Camera::position.y * Camera::scale);

	AEGfxSetTransform(transform.m);
	AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);

	// Reset
	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
	AEGfxSetTransparency(1.f);

	float offset = animateOffset;
	for (auto& i : data)
		offset = i.Print(offset);
}

void Credits::StartCredits()
{
	config.transparency = 0.f;
	Reset();
}

void Credits::DrawInspector()
{
	ImGui::Begin("Credits", &isInspectorOpen);

	ImGui::DragFloat("Animate Offset", &animateOffset, 0.1f);
	ImGui::DragFloat("Section Spacing", &config.sectionSpacing, 0.01f, 0.f, 1.f);
	ImGui::DragFloat("Title Spacing", &config.titleSpacing, 0.01f, 0.f, 1.f);
	ImGui::DragFloat("Names Spacing", &config.namesSpacing, 0.01f, 0.f, 1.f);
	ImGui::DragFloat("Column Spacing", &config.columnSpacing, 0.01f, 0.f, 1.f);

	ImGui::End();
}

// ================
// | Credits Data |
// ================
Credits::CreditsData::CreditsData(const rapidjson::GenericObject<false, rapidjson::Value>& obj, const Config& config):
	config(config)
{
	// === Title ===
	if (obj.HasMember(titleKey))
		title.SetText(obj[titleKey].GetString(), config.fontId);
	else
		title.SetText("", config.fontId);

	// === Names ===
	if (obj.HasMember(namesKey))
	{
		auto& val = obj[namesKey];
		if (val.IsArray())
		{
			auto namesArr = val.GetArray();
			names.reserve(namesArr.Size());

			for (rapidjson::SizeType j = 0; j < namesArr.Size(); j++)
				names.emplace_back(namesArr[j].GetString(), config.fontId);
		}
		else
		{
			names.emplace_back(val.GetString(), config.fontId);
		}
	}

	// === Columns ===
	if (obj.HasMember(columnsKey))
	{
		auto arr = obj[columnsKey].GetArray();
		assert(arr.Size() == 2);
	
		columns.emplace_back(arr[0].GetObj(), config);
		columns.emplace_back(arr[1].GetObj(), config);
	}

	// === Find width (Max width of all text) ===
	size.x = title.size.x;
	
	for (auto& i : names)
		size.x = max(size.x, i.size.x);
	
	for (auto& i : columns)
		size.x = max(size.x, i.size.x);

	// === Find height (Combined height of everything) ===
	size.y = title.size.y + config.titleSpacing;

	for (auto& i : names)
		size.y += config.namesSpacing + i.size.y;

	if (!columns.empty())
		size.y += max(columns[0].size.y, columns[1].size.y);

	size.y += config.sectionSpacing;
}

float Credits::CreditsData::Print(float yOffset, float xOffset)
{
	if (title.HasText())
	{
		title.PrintCenter(config.fontId, xOffset, yOffset, 1.f, 0.82f, 0.35f, config.transparency);
		yOffset -= config.titleSpacing + title.size.y;
	}
	
	for (auto& i : names)
	{
		i.PrintCenter(config.fontId, xOffset, yOffset, 1.f, 1.f, 1.f, config.transparency);
		yOffset -= config.namesSpacing + i.size.y;
	}

	if (!columns.empty())
	{
		float spacing = config.columnSpacing + max(columns[0].size.x, columns[1].size.x) * 0.5f;
		float offset0 = columns[0].Print(yOffset, xOffset - spacing);
		float offset1 = columns[1].Print(yOffset, xOffset + spacing);
		yOffset = max(offset0, offset1);
	}

	return yOffset - config.sectionSpacing;
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
