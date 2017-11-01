#pragma once
#pragma comment(lib, "BakkesMod.lib")
#include "bakkesmod\plugin\bakkesmodplugin.h"
#include "dollycam.h"

class DollyCamPlugin : public BakkesMod::Plugin::BakkesModPlugin
{
private:
	std::shared_ptr<DollyCam> dollyCam;
	bool IsApplicable();
public:
	virtual void onLoad();
	virtual void onUnload();
	void onTick(std::string funcName);
	void OnAllCommand(vector<string> params);
};
