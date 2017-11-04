#include "dollycamplugin.h"
#include "gameapplier.h"
#include "bakkesmod\wrappers\replaywrapper.h"
#include "bakkesmod\wrappers\camerawrapper.h"

#include "utils\parser.h"
#include "serialization.h"

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
	cvarManager->registerCvar("dolly_interpmode", "0", "Used interp mode", true, true, 0, true, 2000).addOnValueChanged(bind(&DollyCamPlugin::OnInterpModeChanged, this, _1, _2));

	cvarManager->registerNotifier("dolly_path_clear", bind(&DollyCamPlugin::OnAllCommand, this, _1));
	cvarManager->registerNotifier("dolly_snapshot_take", bind(&DollyCamPlugin::OnReplayCommand, this, _1));
	cvarManager->registerNotifier("dolly_activate", bind(&DollyCamPlugin::OnReplayCommand, this, _1));
	cvarManager->registerNotifier("dolly_deactivate", bind(&DollyCamPlugin::OnReplayCommand, this, _1));

	cvarManager->registerNotifier("dolly_cam_show", bind(&DollyCamPlugin::OnCamCommand, this, _1));
	cvarManager->registerNotifier("dolly_cam_set_location", bind(&DollyCamPlugin::OnCamCommand, this, _1));
	cvarManager->registerNotifier("dolly_cam_set_rotation", bind(&DollyCamPlugin::OnCamCommand, this, _1));
	cvarManager->registerNotifier("dolly_cam_set_fov", bind(&DollyCamPlugin::OnCamCommand, this, _1));
	cvarManager->registerNotifier("dolly_cam_set_frame", bind(&DollyCamPlugin::OnCamCommand, this, _1));

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
	string command = params.at(0);
	if (command.compare("dolly_path_clear") == 0)
	{
		dollyCam->Reset();
	}
}

void DollyCamPlugin::OnCamCommand(vector<string> params)
{
	if (!IsApplicable())
	{
		cvarManager->log("You cannot use that command here. Make sure you're in flycam");
		return;
	}

	string command = params.at(0);
	CameraWrapper camera = gameWrapper->GetCamera();
	if (command.compare("dolly_cam_show") == 0)
	{
		auto location = camera.GetLocation();
		auto rotation = camera.GetRotation();
		ReplayWrapper replay = gameWrapper->GetGameEventAsReplay();
		cvarManager->log("Frame: " + to_string(replay.GetCurrentReplayFrame()));
		cvarManager->log("Time: " + to_string_with_precision(replay.GetReplayTimeElapsed(), 5));
		cvarManager->log("FOV: " + to_string_with_precision(camera.GetFOV(), 5));
		cvarManager->log("Location " + vector_to_string(location));
		cvarManager->log("Rotation " + rotator_to_string(rotation));
	} 
	else if (command.compare("dolly_cam_set_location") == 0)
	{
		if (params.size() < 4) {
			cvarManager->log("Usage: " + params.at(0) + " x y z");
			return;
		}
		float x = get_safe_float(params.at(1));
		float y = get_safe_float(params.at(2));
		float z = get_safe_float(params.at(3));
		camera.SetLocation({ x,y,z });
	}
	else if (command.compare("dolly_cam_set_rotation") == 0)
	{
		if (params.size() < 4) {
			cvarManager->log("Usage: " + params.at(0) + " pitch yaw roll");
			return;
		}
		float pitch = get_safe_float(params.at(1));
		float yaw = get_safe_float(params.at(2));
		float roll = get_safe_float(params.at(3));
		camera.SetRotation({ pitch, yaw, roll });
	}
	else if (command.compare("dolly_cam_set_frame") == 0)
	{
		/*cvarManager->log("Command not supported, please just edit the json manually");
		return;*/
		if (params.size() < 2) {
			cvarManager->log("Usage: " + params.at(0) + " frame");
			return;
		}
		float timestamp = get_safe_float(params.at(1));
		//gw->GetGameEventAsReplay().SetSecondsElapsed(timestamp);
		gameWrapper->GetGameEventAsReplay().SkipToFrame(timestamp);
	}
	else if (command.compare("dolly_cam_set_fov") == 0)
	{
		if (params.size() < 2) {
			cvarManager->log("Usage: " + params.at(0) + " fov");
			return;
		}
		float fov = get_safe_float(params.at(1));

		camera.SetFOV(fov);
	}
}

void DollyCamPlugin::OnReplayCommand(vector<string> params)
{
	if (!IsApplicable())
	{
		cvarManager->log("You cannot use that command here. Make sure you're in flycam");
		return;
	}
		
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

void DollyCamPlugin::OnInterpModeChanged(string oldValue, CVarWrapper newCvar)
{
	dollyCam->RefreshInterpData();
	cvarManager->log("Now using " + dollyCam->GetInterpolationMethod());
}
