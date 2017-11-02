#include "dollycam.h"
#include "bakkesmod\wrappers\gamewrapper.h"
#include "bakkesmod\wrappers\replaywrapper.h"
#include "bakkesmod\wrappers\camerawrapper.h"

void DollyCam::UpdateRenderPath()
{
	currentRenderPath = make_shared<savetype>(savetype());
	auto strategy = CreateInterpStrategy();
	auto firstFrame = currentPath->begin();
	float beginTime = firstFrame->second.timeStamp;
	int startFrame = firstFrame->first;
	int endFrame = (--currentPath->end())->first;
	
	float replayTickRate = 1.f / 30.f;//Retrieve this from game later

	for (int i = startFrame; i < endFrame; i++)
	{
		CameraSnapshot snapshot;
		snapshot.frame = i;
		snapshot.timeStamp = beginTime + (replayTickRate * (i - startFrame));
		NewPOV pov = strategy->GetPOV(snapshot.timeStamp, i);
		snapshot.location = pov.location;
		snapshot.rotation = pov.rotation;
		snapshot.FOV = pov.FOV;

		currentRenderPath->insert(make_pair(i, snapshot));
	}
}

DollyCam::DollyCam(std::shared_ptr<GameWrapper> _gameWrapper, std::shared_ptr<CVarManagerWrapper> _cvarManager, std::shared_ptr<IGameApplier> _gameApplier)
{
	currentPath = std::unique_ptr<savetype>(new savetype());
	gameWrapper = _gameWrapper;
	cvarManager = _cvarManager;
	gameApplier = _gameApplier;
}

DollyCam::~DollyCam()
{
}

bool DollyCam::TakeSnapshot()
{
	if (!gameWrapper->IsInReplay())
		return false;

	ReplayWrapper sw = gameWrapper->GetGameEventAsReplay();
	CameraWrapper flyCam = gameWrapper->GetCamera();
	if (sw.IsNull())
		return false;

	CameraSnapshot save;
	save.timeStamp = sw.GetReplayTimeElapsed();
	save.FOV = flyCam.GetFOV();
	save.location = flyCam.GetLocation();
	save.rotation = CustomRotator(flyCam.GetRotation());
	save.frame = sw.GetCurrentReplayFrame();
	
	currentPath->insert(std::make_pair(save.frame, save));
	UpdateRenderPath();
}

bool DollyCam::IsActive()
{
	return isActive;
}

void DollyCam::Activate()
{
	if (isActive)
		return;

	isActive = true;

	interpStrategy = CreateInterpStrategy();
	cvarManager->log("Dollycam activated");
}

void DollyCam::Deactivate()
{
	if (!isActive)
		return;
	CameraWrapper flyCam = gameWrapper->GetCamera();
	if (!flyCam.GetCameraAsActor().IsNull()) {
		flyCam.SetLockedFOV(false);
	}
	isActive = false;
	cvarManager->log("Dollycam deactivated");
}

float lastWrite = -5000.f;
float diff = .0f;
bool isFirst = true;
void DollyCam::Apply()
{
	ReplayWrapper sw = gameWrapper->GetGameEventAsReplay();
	int currentFrame = sw.GetCurrentReplayFrame();
	cvarManager->log("Frame: " + to_string(currentFrame) + ". Replay time: " + to_string(sw.GetReplayTimeElapsed()));
	if (currentFrame == currentPath->begin()->first)
	{
		if (isFirst) {
			diff = sw.GetSecondsElapsed();
			isFirst = false;
		}
	}
	else {
		isFirst = true;
	}

	NewPOV pov = interpStrategy->GetPOV(sw.GetSecondsElapsed() - diff + currentPath->begin()->second.timeStamp, sw.GetCurrentReplayFrame());
	if (pov.FOV < 1) { //Invalid camerastate
		return;
	}
	gameApplier->SetPOV(pov.location, pov.rotation, pov.FOV);
	//flyCam.SetPOV(pov.ToPOV());
}

void DollyCam::SetRenderPath(bool render)
{
	renderPath = render;
}

void DollyCam::Render(CanvasWrapper cw)
{
	if (!renderPath || !currentRenderPath || currentRenderPath->size() < 2)
		return;
	Vector2 prevLine = cw.Project(currentRenderPath->begin()->second.location);
	Vector2 canvasSize = cw.GetSize();
	for (auto it = (++currentRenderPath->begin()); it != currentRenderPath->end(); ++it)
	{
		Vector2 line = cw.Project(it->second.location);

		cw.SetColor(255, 255, 1, 122);

		line.X = max(0, line.X);
		line.X = min(line.X, canvasSize.X);
		line.Y = max(0, line.Y);
		line.Y = min(line.Y, canvasSize.Y);
		if (!((line.X < .1 || line.X >= canvasSize.X - .5) && (line.Y < .1 || line.Y >= canvasSize.Y - .5)))
		{
			cw.DrawLine(prevLine, line);
		}
		
		prevLine = line;
	}

	for (auto it = currentPath->begin(); it != currentPath->end(); it++)
	{
		auto boxLoc = cw.Project(it->second.location);
		cw.SetColor(255, 0, 0, 255);
		if (boxLoc.X >= 0 && boxLoc.X <= canvasSize.X && boxLoc.Y >= 0 && boxLoc.Y <= canvasSize.Y) {
			boxLoc.X -= 5;
			boxLoc.Y -= 5;
			cw.SetPosition(boxLoc);
			cw.DrawBox({ 5,5 });
		}
	}
}

shared_ptr<InterpStrategy> DollyCam::CreateInterpStrategy()
{
	CVarWrapper interpMode = cvarManager->getCvar("dolly_interpmode");
	switch (interpMode.getIntValue())
	{
	case 0:
		return std::make_shared<LinearInterpStrategy>(LinearInterpStrategy(currentPath));
		break;
	}

	cvarManager->log("Interpstrategy not found, defaulting to linear interp");
	return std::make_shared<LinearInterpStrategy>(LinearInterpStrategy(currentPath));;
}
