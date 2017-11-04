#pragma once
#pragma comment(lib, "BakkesMod.lib")
#include "bakkesmod\plugin\bakkesmodplugin.h"
#include "dollycam.h"

class DollyCamPlugin : public BakkesMod::Plugin::BakkesModPlugin
{
private:
	std::shared_ptr<DollyCam> dollyCam;
	std::shared_ptr<bool> renderCameraPath;
	bool IsApplicable();
public:
	virtual void onLoad();
	virtual void onUnload();
	void onTick(std::string funcName);
	void OnAllCommand(vector<string> params);
	void OnCamCommand(vector<string> params);
	void OnReplayCommand(vector<string> params);
	void onRender(CanvasWrapper canvas);
	void OnInterpModeChanged(string oldValue, CVarWrapper newCvar);
};
