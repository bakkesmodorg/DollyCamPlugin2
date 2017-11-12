#include "dollycam.h"
#include "bakkesmod\wrappers\gamewrapper.h"
#include "bakkesmod\wrappers\replaywrapper.h"
#include "bakkesmod\wrappers\camerawrapper.h"
#include "utils/parser.h"

#include "interpstrategies\supportedstrategies.h"

void DollyCam::UpdateRenderPath()
{
	currentRenderPath = make_shared<savetype>(savetype());
	auto strategy = CreateInterpStrategy();
	auto firstFrame = currentPath->begin();
	float beginTime = firstFrame->second.timeStamp;

	int startFrame = firstFrame->first;
	int endFrame = (--currentPath->end())->first;
	
	float replayTickRate = 1.f / (float)gameWrapper->GetGameEventAsReplay().GetReplayFPS();

	int lastSyncedFrame = startFrame;
	float timePerFrame = replayTickRate;
	for (int i = startFrame; i <= endFrame; i++)
	{
		if (currentPath->find(i) != currentPath->end())
		{
			lastSyncedFrame = i;
			auto currentSnapshot = currentPath->find(i);
			beginTime = currentSnapshot->second.timeStamp;
			timePerFrame = replayTickRate;
			//auto nextSnapshot = currentPath->upper_bound(i);
			//timePerFrame = (nextSnapshot->second.timeStamp - beginTime) / (nextSnapshot->second.frame - currentSnapshot->second.frame);
			//if (timePerFrame < .01f || timePerFrame > .08f) //outliers
			//	timePerFrame = replayTickRate;
		}
		


		CameraSnapshot snapshot;
		snapshot.frame = i;
		snapshot.timeStamp = beginTime + (timePerFrame * (i - lastSyncedFrame));
		NewPOV pov = strategy->GetPOV(snapshot.timeStamp, i);
		snapshot.location = pov.location;
		snapshot.rotation = pov.rotation;
		snapshot.FOV = pov.FOV;

		if(snapshot.FOV > 1)
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

CameraSnapshot DollyCam::TakeSnapshot(bool saveToPath)
{
	CameraSnapshot save;
	if (!gameWrapper->IsInReplay())
		return save;

	ReplayWrapper sw = gameWrapper->GetGameEventAsReplay();
	CameraWrapper flyCam = gameWrapper->GetCamera();
	if (sw.IsNull())
		return save;
	
	save.timeStamp = sw.GetReplayTimeElapsed();
	save.FOV = flyCam.GetFOV();
	save.location = flyCam.GetLocation();
	save.rotation = CustomRotator(flyCam.GetRotation());
	save.frame = sw.GetCurrentReplayFrame();
	
	if (saveToPath) {
		this->InsertSnapshot(save);
	}

	return save;
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
	if (currentFrame < currentPath->begin()->first || currentFrame > (--currentPath->end())->first)
		return;
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

void DollyCam::Reset()
{
	this->currentPath->clear();
	this->RefreshInterpData();
}

void DollyCam::InsertSnapshot(CameraSnapshot snapshot)
{
	this->currentPath->insert_or_assign(snapshot.frame, snapshot);
	this->RefreshInterpData();
}

bool DollyCam::IsFrameUsed(int frame)
{
	return currentPath->find(frame) != currentPath->end();
}

CameraSnapshot DollyCam::GetSnapshot(int frame)
{
	CameraSnapshot snapshot;
	auto it = currentPath->find(frame);
	if (it != currentPath->end())
	{
		snapshot = it->second;
	}
	return snapshot;
}

void DollyCam::DeleteFrame(int frame)
{
	auto it = currentPath->find(frame);
	if (it != currentPath->end())
		this->currentPath->erase(it);
	this->RefreshInterpData();
}

vector<int> DollyCam::GetUsedFrames()
{
	vector<int> frames = vector<int>();
	for (auto it = currentPath->begin(); it != currentPath->end(); it++)
	{
		frames.push_back(it->first);
	}
	return frames;
}

void DollyCam::SetRenderPath(bool render)
{
	renderPath = render;
}

void DollyCam::SetRenderFrames(bool _renderFrames)
{
	this->renderFrames = _renderFrames;
}

void DollyCam::Render(CanvasWrapper cw)
{
	if (!renderPath || !currentRenderPath || currentRenderPath->size() < 2)
		return;

	ReplayWrapper sw = gameWrapper->GetGameEventAsReplay();
	int currentFrame = sw.GetCurrentReplayFrame();

	Vector2 prevLine = cw.Project(currentRenderPath->begin()->second.location);
	Vector2 canvasSize = cw.GetSize();

	int colTest = 255;
	for (auto it = (++currentRenderPath->begin()); it != currentRenderPath->end(); ++it)
	{
		Vector2 line = cw.Project(it->second.location);

		if (it->first - 2 < currentFrame && it->first + 2 > currentFrame)
		{
			cw.SetColor(255, 0, 0, 255);
		}
		else
		{
			cw.SetColor(0, 0, colTest, 255);
			colTest = colTest == 0 ? 255 : 255;
		}

		line.X = max(0, line.X);
		line.X = min(line.X, canvasSize.X);
		line.Y = max(0, line.Y);
		line.Y = min(line.Y, canvasSize.Y);
		if (!(((line.X < .1 || line.X >= canvasSize.X - .5) && (line.Y < .1 || line.Y >= canvasSize.Y - .5)) || ((prevLine.X < .1 || prevLine.X >= canvasSize.X - .5) && (prevLine.Y < .1 || prevLine.Y >= canvasSize.Y - .5))))
		{
			cw.DrawLine(prevLine, line);
			cw.DrawLine(prevLine.minus({ 1,1 }), line.minus({ 1,1 })); //make lines thicker
			cw.DrawLine(prevLine.minus({ -1,-1 }), line.minus({ -1,-1 }));
			if (renderFrames) {
				cw.SetColor(0, 0, 0, 255);
				cw.SetPosition(line);
				cw.DrawString(to_string(it->first));
			}
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
			cw.FillBox({ 10, 10});
			cw.SetColor(0, 0, 0, 255);
			cw.DrawString(to_string(it->first) + " (w:" + to_string_with_precision(it->second.weight, 2) + ")");
		}
	}
}

void DollyCam::RefreshInterpData()
{
	interpStrategy = CreateInterpStrategy();
	UpdateRenderPath();
}

string DollyCam::GetInterpolationMethod()
{
	if (interpStrategy)
		return interpStrategy->GetName();
	return "none";
}



shared_ptr<InterpStrategy> DollyCam::CreateInterpStrategy()
{
	CVarWrapper interpMode = cvarManager->getCvar("dolly_interpmode");
	int chaikinDegree = cvarManager->getCvar("dolly_chaikin_degree").getIntValue();
	switch (interpMode.getIntValue())
	{
	case 0:
		return std::make_shared<LinearInterpStrategy>(LinearInterpStrategy(currentPath, chaikinDegree));
		break;
	case 1:
		return std::make_shared<NBezierInterpStrategy>(NBezierInterpStrategy(currentPath, chaikinDegree));
		break;
	case 2:
		return std::make_shared<CosineInterpStrategy>(CosineInterpStrategy(currentPath));
		break;
	case 3:
		return std::make_shared<HermiteInterpStrategy>(HermiteInterpStrategy(currentPath));
		break;
	case 4:
		return std::make_shared<CatmullRomInterpStrategy>(CatmullRomInterpStrategy(currentPath));
		break;
	}

	cvarManager->log("Interpstrategy not found!!! Defaulting to linear interp.");
	return std::make_shared<LinearInterpStrategy>(LinearInterpStrategy(currentPath, chaikinDegree));
}
