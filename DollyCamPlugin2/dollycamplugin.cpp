#include "dollycamplugin.h"
#include "gameapplier.h"

#include "bakkesmod\wrappers\camerawrapper.h"
using namespace std::placeholders;

BAKKESMOD_PLUGIN(DollyCamPlugin, "Dollycam plugin", "2", PLUGINTYPE_REPLAY | PLUGINTYPE_SPECTATOR)

bool DollyCamPlugin::IsApplicable()
{
	return gameWrapper->IsInReplay() && gameWrapper->GetCamera().GetCameraState().compare("CameraState_ReplayFly_TA") == 0;
}

void DollyCamPlugin::onLoad()
{
	std::shared_ptr<IGameApplier> gameApplier = std::make_shared<RealGameApplier>(RealGameApplier(gameWrapper));
	dollyCam = std::make_shared<DollyCam>(DollyCam(gameWrapper, cvarManager, gameApplier));

	gameWrapper->HookEvent("Function TAGame.CameraState_ReplayFly_TA.UpdatePOV", bind(&DollyCamPlugin::onTick, this, _1));
	gameWrapper->RegisterDrawable(bind(&DollyCamPlugin::onRender, this, _1));
	cvarManager->registerCvar("dolly_interpmode", "0", "Used interp mode", true, true, 0, true, 2000);

	cvarManager->registerNotifier("dolly_path_clear", bind(&DollyCamPlugin::OnAllCommand, this, _1));
	cvarManager->registerNotifier("dolly_snapshot_take", bind(&DollyCamPlugin::OnReplayCommand, this, _1));
	cvarManager->registerNotifier("dolly_activate", bind(&DollyCamPlugin::OnReplayCommand, this, _1));
	cvarManager->registerNotifier("dolly_deactivate", bind(&DollyCamPlugin::OnReplayCommand, this, _1));
	dollyCam->SetRenderPath(true);
}

void DollyCamPlugin::onUnload()
{
}


void DollyCamPlugin::onTick(std::string funcName)
{
	if (!IsApplicable() || !dollyCam->IsActive())
		return;
	dollyCam->Apply();
}

//[&cm = this->cvarManager, &gw = this->gameWrapper](vector<string>)
void DollyCamPlugin::OnAllCommand(vector<string> params)
{

}

void DollyCamPlugin::OnReplayCommand(vector<string> params)
{
	if (!IsApplicable())
		return;
	string command = params.at(0);
	if (command.compare("dolly_snapshot_take") == 0)
	{
		dollyCam->TakeSnapshot();
	}
	else if (command.compare("dolly_deactivate") == 0)
	{
		dollyCam->Deactivate();
	}
	else if (command.compare("dolly_activate") == 0)
	{
		dollyCam->Activate();
	}

}

void DollyCamPlugin::onRender(CanvasWrapper canvas)
{
	if (!IsApplicable())
		return;
	dollyCam->Render(canvas);
}
