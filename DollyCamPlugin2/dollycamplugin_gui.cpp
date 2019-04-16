#ifdef PLUGIN_GUI
#include "dollycamplugin.h"

#include "imgui.h"

void DollyCamPlugin::Render()
{
	ImGui::Text("ImGui stuff from inside plugin");
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

#endif