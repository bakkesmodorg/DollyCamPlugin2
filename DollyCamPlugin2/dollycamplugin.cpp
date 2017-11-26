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
	renderCameraPath = std::make_shared<bool>(true);

	gameWrapper->HookEvent("Function TAGame.CameraState_ReplayFly_TA.UpdatePOV", bind(&DollyCamPlugin::onTick, this, _1));
	gameWrapper->RegisterDrawable(bind(&DollyCamPlugin::onRender, this, _1));

	cvarManager->registerCvar("dolly_interpmode", "0", "Used interp mode", true, true, 0, true, 2000).addOnValueChanged(bind(&DollyCamPlugin::OnInterpModeChanged, this, _1, _2));
	
	cvarManager->registerCvar("dolly_interpmode_location", "0", "Used interp mode for location", true, true, 0, true, 2000).addOnValueChanged(bind(&DollyCamPlugin::OnInterpModeChanged, this, _1, _2));
	cvarManager->registerCvar("dolly_interpmode_rotation", "0", "Used interp mode for rotation", true, true, 0, true, 2000).addOnValueChanged(bind(&DollyCamPlugin::OnInterpModeChanged, this, _1, _2));

	
	cvarManager->registerCvar("dolly_render", "1", "Render the current camera path", true, true, 0, true, 1).bindTo(renderCameraPath);
	cvarManager->registerCvar("dolly_render_frame", "1", "Render frame numbers on the path", true, true, 0, true, 1).addOnValueChanged(bind(&DollyCamPlugin::OnRenderFramesChanged, this, _1, _2));

	cvarManager->registerNotifier("dolly_path_clear", bind(&DollyCamPlugin::OnAllCommand, this, _1));
	cvarManager->registerNotifier("dolly_snapshot_take", bind(&DollyCamPlugin::OnReplayCommand, this, _1));
	cvarManager->registerNotifier("dolly_activate", bind(&DollyCamPlugin::OnReplayCommand, this, _1));
	cvarManager->registerNotifier("dolly_deactivate", bind(&DollyCamPlugin::OnReplayCommand, this, _1));
	
	cvarManager->registerNotifier("dolly_cam_show", bind(&DollyCamPlugin::OnCamCommand, this, _1));
	cvarManager->registerNotifier("dolly_cam_set_location", bind(&DollyCamPlugin::OnCamCommand, this, _1));
	cvarManager->registerNotifier("dolly_cam_set_rotation", bind(&DollyCamPlugin::OnCamCommand, this, _1));
	cvarManager->registerNotifier("dolly_cam_set_fov", bind(&DollyCamPlugin::OnCamCommand, this, _1));
	cvarManager->registerNotifier("dolly_cam_set_frame", bind(&DollyCamPlugin::OnCamCommand, this, _1));

	cvarManager->registerNotifier("dolly_snapshot_list", bind(&DollyCamPlugin::OnSnapshotCommand, this, _1));
	cvarManager->registerNotifier("dolly_snapshot_info", bind(&DollyCamPlugin::OnSnapshotCommand, this, _1));
	//cvarManager->registerNotifier("dolly_snapshot_set", bind(&DollyCamPlugin::OnSnapshotCommand, this, _1));
	cvarManager->registerNotifier("dolly_snapshot_override", bind(&DollyCamPlugin::OnSnapshotCommand, this, _1));
	cvarManager->registerNotifier("dolly_snapshot_delete", bind(&DollyCamPlugin::OnSnapshotCommand, this, _1));

	cvarManager->registerNotifier("dolly_bezier_weight", bind(&DollyCamPlugin::OnBezierCommand, this, _1));
	cvarManager->registerCvar("dolly_chaikin_degree", "0", "Amount of times to apply chaikin to the spline", true, true, 0, true, 20).addOnValueChanged(bind(&DollyCamPlugin::OnChaikinChanged, this, _1, _2));;

	dollyCam->SetRenderPath(true);
}

void DollyCamPlugin::onUnload()
{
}

void DollyCamPlugin::PrintSnapshotInfo(CameraSnapshot shot)
{
	cvarManager->log("Frame: " + to_string(shot.frame));
	cvarManager->log("Time: " + to_string_with_precision(shot.timeStamp, 5));
	cvarManager->log("FOV: " + to_string_with_precision(shot.FOV, 5));
	cvarManager->log("Location " + vector_to_string(shot.location));
	cvarManager->log("Rotation " + rotator_to_string(shot.rotation.ToRotator()));
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
		CameraSnapshot cameraInfo = dollyCam->TakeSnapshot(false);
		this->PrintSnapshotInfo(cameraInfo);
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
		if (params.size() < 2) {
			cvarManager->log("Usage: " + params.at(0) + " frame");
			return;
		}
		float timestamp = get_safe_int(params.at(1));
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
		CameraSnapshot snapshot = dollyCam->TakeSnapshot(true);
		if (snapshot.frame < 0)
		{
			cvarManager->log("Failed to create snapshot");
		}
		else
		{
			cvarManager->log("Saved snapshot #" + to_string(snapshot.frame));
			cvarManager->executeCommand("dolly_snapshot_info " + to_string(snapshot.frame), false);
		}
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

void DollyCamPlugin::OnSnapshotCommand(vector<string> params)
{
	if (!IsApplicable())
	{
		cvarManager->log("You cannot do that here!");
		return;
	}

	string command = params.at(0);
	if (command.compare("dolly_snapshot_list") == 0)
	{
		vector<int> frames = dollyCam->GetUsedFrames();
		for (auto it = frames.begin(); it != frames.end(); it++)
		{
			CameraSnapshot snapshot = dollyCam->GetSnapshot(*it);
			cvarManager->log("ID: " + to_string(snapshot.frame) + ", [" + to_string_with_precision(snapshot.weight, 2) + "][" + to_string_with_precision(snapshot.timeStamp, 2) + "][" + to_string_with_precision(snapshot.FOV, 2) + "] (" + vector_to_string(snapshot.location) + ") (" + rotator_to_string(snapshot.rotation.ToRotator()) + " )");
		}
		cvarManager->log("Current path has " + to_string(frames.size()) + " snapshots.");
	} 
	else if (command.compare("dolly_snapshot_info") == 0)
	{
		if (params.size() < 2) {
			cvarManager->log("Usage: " + params.at(0) + " id");
			return;
		}
		int frame = get_safe_int(params.at(1));
		if (!dollyCam->IsFrameUsed(frame)) {
			cvarManager->log("This snapshot does not exist");
		}
		else 
		{
			CameraSnapshot snapshot = dollyCam->GetSnapshot(frame);
			cvarManager->log("ID " + to_string(snapshot.frame) + ". FOV: " + to_string(snapshot.FOV) + ". Time: " + to_string_with_precision(snapshot.timeStamp, 3) + ". Weight: " + to_string_with_precision(snapshot.weight, 3));
			cvarManager->log("Location " + vector_to_string(snapshot.location));
			cvarManager->log("Rotation " + rotator_to_string(snapshot.rotation.ToRotator()));
			if (params.size() == 3) {
				string action = params.at(2);
				if (action.compare("set") == 0) {
					CameraWrapper camera = gameWrapper->GetCamera();
					camera.SetLocation(snapshot.location);
					camera.SetRotation(snapshot.rotation.ToRotator());
					camera.SetFOV(snapshot.FOV);
				}
			}
		}
	}
	else if (command.compare("dolly_snapshot_set") == 0)
	{
		if (params.size() < 2) {
			cvarManager->log("Usage: " + params.at(0) + " id");
			return;
		}
		cvarManager->executeCommand("dolly_snapshot_info " + params.at(1) + " set", false);

	}
	else if (command.compare("dolly_snapshot_override") == 0)
	{
		if (params.size() < 2) {
			cvarManager->log("Usage: " + params.at(0) + " id");
			return;
		}

		int frame = get_safe_int(params.at(1));
		/*if (!dollyCam->IsFrameUsed(frame)) {
			cvarManager->log("This snapshot does not exist");
			return;
		}*/
		CameraSnapshot snapshot = dollyCam->TakeSnapshot(false);
		snapshot.frame = frame;
		dollyCam->InsertSnapshot(snapshot);
	}
	else if (command.compare("dolly_snapshot_delete") == 0)
	{
		if (params.size() < 2) {
			cvarManager->log("Usage: " + params.at(0) + " id");
			return;
		}
		int id = get_safe_int(params.at(1));
		dollyCam->DeleteFrame(id);
	}
}

void DollyCamPlugin::onRender(CanvasWrapper canvas)
{
	if (!IsApplicable() || !*renderCameraPath)
		return;
	dollyCam->Render(canvas);
}

void DollyCamPlugin::OnInterpModeChanged(string oldValue, CVarWrapper newCvar)
{
	string cvarName = newCvar.getCVarName();
	if (cvarName.compare("dolly_interpmode") == 0)
	{
		cvarManager->executeCommand("dolly_interpmode_location " + newCvar.getStringValue(), false);
		cvarManager->executeCommand("dolly_interpmode_rotation " + newCvar.getStringValue(), false);
	}
	else if(cvarName.compare("dolly_interpmode_location") == 0)
	{
		dollyCam->RefreshInterpData();
		cvarManager->log("Now using " + dollyCam->GetInterpolationMethod(true) + " for camera location.");
	}
	else if(cvarName.compare("dolly_interpmode_rotation") == 0)
	{
		dollyCam->RefreshInterpDataRotation();
		cvarManager->log("Now using " + dollyCam->GetInterpolationMethod(false) + " for camera rotation.");
	}
}

void DollyCamPlugin::OnRenderFramesChanged(string oldValue, CVarWrapper newCvar)
{
	dollyCam->SetRenderFrames(newCvar.getBoolValue());
}

void DollyCamPlugin::OnChaikinChanged(string oldValue, CVarWrapper newCvar)
{
	dollyCam->RefreshInterpData();
}

void DollyCamPlugin::OnBezierCommand(vector<string> params)
{
	string command = params.at(0);
	if (command.compare("dolly_bezier_weight") == 0)
	{
		if (params.size() < 2)
		{
			cvarManager->log("Usage: " + command + " id weight");
			return;
		}
		int id = get_safe_int(params.at(1));
		if (!dollyCam->IsFrameUsed(id))
		{
			cvarManager->log("Frame #" + to_string(id) + " does not have a snapshot attached");
			return;
		}

		CameraSnapshot snapshot = dollyCam->GetSnapshot(id);
		if (params.size() == 2)
		{
			cvarManager->log("Snapshot #" + to_string(snapshot.frame) + " weight: " + to_string(snapshot.weight));
		}
		else {
			float weight = get_safe_float(params.at(2));
			snapshot.weight = weight;
			dollyCam->InsertSnapshot(snapshot);
			cvarManager->log("Snapshot #" + to_string(snapshot.frame) + " saved with weight: " + to_string(snapshot.weight));
		}
	}

}
