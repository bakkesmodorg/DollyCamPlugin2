#include "dollycamplugin.h"

#include "imgui\imgui.h"
#include "imgui\imgui_internal.h"
#include "serialization.h"
#include "bakkesmod\..\\utils\parser.h"
namespace COLWIDTHS
{
	const int ID = 25;
	const int FRAME = 40;
	const int TIMESTAMP = 60;
	const int LOCATION = 200;
	const int ROTATION = 140;
	const int FOV = 40;
	const int REMOVEBUTTON = 60;
	const int TOTAL = ID + FRAME + TIMESTAMP + LOCATION + ROTATION + FOV + REMOVEBUTTON;
}

void DollyCamPlugin::Render()
{
	ImGui::SetNextWindowSizeConstraints(ImVec2(COLWIDTHS::TOTAL, 600), ImVec2(FLT_MAX, FLT_MAX));

	//setting bg alpha to 0.75
	auto context = ImGui::GetCurrentContext();
	const ImGuiCol bg_color_idx = ImGuiCol_WindowBg;
	//const ImVec4 bg_color_backup = context->Style.Colors[bg_color_idx];
	context->Style.Colors[bg_color_idx].w = 0.75;

	string menuName = "Snapshots";
	//if (!ImGui::Begin(menuName.c_str(), &isWindowOpen))
	//{
	//	// Early out if the window is collapsed, as an optimization.
	//	ImGui::End();
	//	return;
	//}
	ImGui::Begin(menuName.c_str(), &isWindowOpen);

	ImGui::BeginChild("#CurrentSnapshotsTab", ImVec2(55 + 250 + 55 + 250, -ImGui::GetFrameHeightWithSpacing()));
	ImGui::Columns(7, "snapshots"); 

	ImGui::SetColumnWidth(0, COLWIDTHS::ID); 
	ImGui::SetColumnWidth(1, COLWIDTHS::FRAME);
	ImGui::SetColumnWidth(2, COLWIDTHS::TIMESTAMP);
	ImGui::SetColumnWidth(3, COLWIDTHS::LOCATION);
	ImGui::SetColumnWidth(4, COLWIDTHS::ROTATION); 
	ImGui::SetColumnWidth(5, COLWIDTHS::FOV); 
	ImGui::SetColumnWidth(6, COLWIDTHS::REMOVEBUTTON); 

	ImGui::Separator();

	ImGui::Text("#"); ImGui::NextColumn();
	ImGui::Text("Frame"); ImGui::NextColumn();
	ImGui::Text("Time"); ImGui::NextColumn();
	ImGui::Text("Location"); ImGui::NextColumn();	
	ImGui::Text("Rotation"); ImGui::NextColumn();
	ImGui::Text("FOV"); ImGui::NextColumn();
	ImGui::Text("Remove"); ImGui::NextColumn();

	ImGui::Separator();

	int index = 1;
	for (const auto& data : *dollyCam->GetCurrentPath())
	{
		auto snapshot = data.second;
		ImGui::Text("%i", index); ImGui::NextColumn();
		ImGui::Text("%i", snapshot.frame); ImGui::NextColumn();
		ImGui::Text(to_string_with_precision(snapshot.timeStamp, 2).c_str()); ImGui::NextColumn();
		ImGui::Text(vector_to_string(snapshot.location).c_str()); ImGui::NextColumn();
		ImGui::Text(rotator_to_string(snapshot.rotation.ToRotator()).c_str()); ImGui::NextColumn();
		ImGui::Text(to_string_with_precision(snapshot.FOV, 1).c_str()); ImGui::NextColumn();
		string buttonIdentifier = "Remove##" + to_string(index);
		if (ImGui::Button(buttonIdentifier.c_str()))
		{
			cvarManager->log("Button pressed");
			dollyCam->DeleteFrameByIndex(index);
		}ImGui::NextColumn();
		ImGui::Separator();
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

