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

	gameWrapper->HookEvent("Function TAGame.PlayerController_TA.PrePhysicsStep", bind(&DollyCamPlugin::onTick, this, _1));
	cvarManager->registerCvar("rebound_shotspeed", "780", "Shotspeed to use for rebounds", true, true, 0, true, 2000);
	cvarManager->registerCvar("rebound_addedheight", "(300, 1400)", "Height above the backboard to shoot", true, true, -5000, true, 10000);

	cvarManager->registerNotifier("dolly_path_clear", bind(&DollyCamPlugin::OnAllCommand, this, _1));
}

void DollyCamPlugin::onUnload()
{
}


void DollyCamPlugin::onTick(std::string funcName)
{
	if (!IsApplicable())
		return;
	CameraWrapper camera = gameWrapper->GetCamera();
}

//[&cm = this->cvarManager, &gw = this->gameWrapper](vector<string>)
void DollyCamPlugin::OnAllCommand(vector<string> params)
{

}