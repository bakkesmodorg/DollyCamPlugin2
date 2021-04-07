#include "dollycamplugin.h"
#include "imgui\imgui.h"
#include "imgui\imgui_internal.h"
#include "serialization.h"
#include "bakkesmod\..\\utils\parser.h"
#include <functional>
#include <vector>
#include <algorithm>

namespace Columns
{
	struct TableColumns
	{
		std::string header;
		int width;
		bool enabled;
		bool widget;
		std::function<std::string(CameraSnapshot, int)> ToString;
		std::function<void(std::shared_ptr<DollyCam>, CameraSnapshot, int)> WidgetCode;

		int GetWidth() const { return enabled ? width : 0; }
		void RenderItem(std::shared_ptr<DollyCam> dollyCam, CameraSnapshot snap, int i) const {
			if (!enabled) return;
			if (widget) 
			{
				WidgetCode(dollyCam, snap, i);
			}
			else {
				ImGui::Text(ToString(snap, i).c_str());
			}
		}
	};
	auto noWidget = [](std::shared_ptr<DollyCam> dollyCam, CameraSnapshot snap, int i) {};
	std::vector< TableColumns > column {
		{"#",			25,		true, false, [](CameraSnapshot snap, int i) {return std::to_string(i); },								noWidget},
		{"Frame",		40,		true, false, [](CameraSnapshot snap, int i) {return std::to_string(snap.frame); },						noWidget},
		{"Time",		60,		true, false, [](CameraSnapshot snap, int i) {return to_string_with_precision(snap.timeStamp, 2); }, noWidget},
		{"Location",	200,	true, false, [](CameraSnapshot snap, int i) {return vector_to_string(snap.location); },				noWidget},
		{"Rotation",	140,	true, false, [](CameraSnapshot snap, int i) {return rotator_to_string(snap.rotation.ToRotator()); },noWidget},
		{"FOV",			40,		true, false, [](CameraSnapshot snap, int i) {return to_string_with_precision(snap.FOV, 1); },		noWidget},
		{"Remove",		80,		true, true, [](CameraSnapshot snap, int i) {return ""; },
			[](std::shared_ptr<DollyCam> dollyCam, CameraSnapshot snap, int i) {
				std::string buttonIdentifier = "Remove##" + std::to_string(i);
				if (ImGui::Button(buttonIdentifier.c_str()))
				{
					dollyCam->DeleteFrameByIndex(i);
				}
			}}
	};
}


void DollyCamPlugin::Render()
{
	auto columns = Columns::column;
	int totalWidth = std::accumulate(columns.begin(), columns.end(), 0, [](int sum, const Columns::TableColumns& element) {return sum + element.GetWidth(); });
	ImGui::SetNextWindowSizeConstraints(ImVec2(totalWidth, 300), ImVec2(FLT_MAX, FLT_MAX));

	//setting bg alpha to 0.75
	auto context = ImGui::GetCurrentContext();
	const ImGuiCol bg_color_idx = ImGuiCol_WindowBg;
	context->Style.Colors[bg_color_idx].w = 0.75;

	std::string menuName = "Snapshots";
	if (!ImGui::Begin(menuName.c_str(), &isWindowOpen, ImGuiWindowFlags_ResizeFromAnySide))
	{
		// Early out if the window is collapsed, as an optimization.
		block_input = ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
		ImGui::End();
		return;
	}

	int enabledCount = std::accumulate(columns.begin(), columns.end(), 0, [](int sum, const Columns::TableColumns& element) {return sum + element.enabled; });
	ImGui::BeginChild("#CurrentSnapshotsTab", ImVec2(totalWidth, -ImGui::GetFrameHeightWithSpacing()));
	ImGui::Columns(enabledCount, "snapshots");

	//Set column widths 
	int index = 0;
	for (const auto& col : columns)
	{
		if (col.enabled)
		{
		ImGui::SetColumnWidth(index, col.width); 
		}
		index++;
	}

	ImGui::Separator();

	//Set column headers
	for (const auto& col : columns)
	{
		if (col.enabled)
		{
			ImGui::Text(col.header.c_str()); 
			ImGui::NextColumn();
		}
		index++;
	}

	ImGui::Separator();

	//Write rows of data
	index = 1;
	for (const auto& data : *dollyCam->GetCurrentPath())
	{
		auto snapshot = data.second;
		for (const auto& col : columns)
		{
			if (col.enabled)
			{
				col.RenderItem(dollyCam, snapshot, index);
				ImGui::NextColumn();
			}
		}
		index++;
	}

	ImGui::EndChild();
	ImGui::End();

	if (!isWindowOpen)
	{
		cvarManager->executeCommand("togglemenu " + GetMenuName());
	}
	block_input = ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;

}

std::string DollyCamPlugin::GetMenuName()
{
	return "dollycam";
}

std::string DollyCamPlugin::GetMenuTitle()
{
	return "Dollycam";
}

void DollyCamPlugin::SetImGuiContext(uintptr_t ctx)
{
	ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
}

bool DollyCamPlugin::ShouldBlockInput()
{
	return block_input;
}

bool DollyCamPlugin::IsActiveOverlay()
{
	return true;
}

void DollyCamPlugin::OnOpen()
{
	isWindowOpen = true;
}

void DollyCamPlugin::OnClose()
{
	isWindowOpen = false;
}

