#include "dollycam.h"
#include "bakkesmod\wrappers\gamewrapper.h"
#include "bakkesmod\wrappers\replayserverwrapper.h"
#include "bakkesmod\wrappers\GameObject\CameraWrapper.h"
#include "utils/parser.h"

#include "interpstrategies\supportedstrategies.h"
#include "serialization.h"
#include <fstream>

void DollyCam::UpdateRenderPath()
{
	if (!gameWrapper->IsInReplay())
		return;
	currentRenderPath = std::make_shared<savetype>(savetype());
	CVarWrapper interpMode = cvarManager->getCvar("dolly_interpmode_location");
	auto locationRenderStrategy = CreateInterpStrategy(interpMode.getIntValue());
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
		NewPOV pov = locationRenderStrategy->GetPOV(snapshot.timeStamp, i);
		snapshot.location = pov.location;
		snapshot.rotation = pov.rotation;
		snapshot.FOV = pov.FOV;

		if (snapshot.FOV > 1)
			currentRenderPath->insert(std::make_pair(i, snapshot));

	}
}

void DollyCam::CheckIfSameInterp()
{
	usesSameInterp = cvarManager->getCvar("dolly_interpmode_location").getIntValue() == cvarManager->getCvar("dolly_interpmode_rotation").getIntValue();
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

	ReplayServerWrapper sw = gameWrapper->GetGameEventAsReplay();
	auto replay = sw.GetReplay();
	CameraWrapper flyCam = gameWrapper->GetCamera();
	if (sw.IsNull())
		return save;


	save.timeStamp = sw.GetReplayTimeElapsed();
	//save.timeStamp = sw.GetCurrentReplayFrame()/replay.GetRecordFPS();
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
	CVarWrapper interpMode = cvarManager->getCvar("dolly_interpmode_location");
	locationInterpStrategy = CreateInterpStrategy(interpMode.getIntValue());
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
	//cvarManager->log("Wa: ");
	int currentFrame = 0;
	ServerWrapper sw(NULL);
	if (gameWrapper->IsInReplay())
	{
		currentFrame = gameWrapper->GetGameEventAsReplay().GetCurrentReplayFrame();
		sw = gameWrapper->GetGameEventAsReplay();
	}
	else if (gameWrapper->IsInGame())
	{
		sw = gameWrapper->GetGameEventAsServer();
		currentFrame = sw.GetReplayDirector().GetReplay().GetCurrentFrame();
	}
	else if (gameWrapper->IsInOnlineGame())
	{
		sw = gameWrapper->GetOnlineGame();
		currentFrame = sw.GetReplayDirector().GetReplay().GetCurrentFrame();
	}
	else
	{
		return;
	}
	//cvarManager->log("Frame: " + std::to_string(currentFrame));
	if (currentFrame < currentPath->begin()->first || currentFrame >(--currentPath->end())->first)
		return;
	//cvarManager->log("Can play: ");
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
	
	NewPOV pov = locationInterpStrategy->GetPOV(sw.GetSecondsElapsed() - diff + currentPath->begin()->second.timeStamp, currentFrame);
	if (!usesSameInterp)
	{
		NewPOV secondaryPov = rotationInterpStrategy->GetPOV(sw.GetSecondsElapsed() - diff + currentPath->begin()->second.timeStamp, currentFrame);
		pov.rotation = secondaryPov.rotation;
		pov.FOV = secondaryPov.FOV;
	}
	if (pov.FOV < 1) { //Invalid camerastate
		return;
	}
	//cvarManager->log("Applying: ");
	gameApplier->SetPOV(pov.location, pov.rotation, pov.FOV);
	//flyCam.SetPOV(pov.ToPOV());
}

void DollyCam::Reset()
{
	this->currentPath->clear();
	this->RefreshInterpData();
	this->RefreshInterpDataRotation();
}

void DollyCam::InsertSnapshot(CameraSnapshot snapshot)
{
	this->currentPath->insert_or_assign(snapshot.frame, snapshot);
	this->RefreshInterpData();
	this->RefreshInterpDataRotation();
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

void DollyCam::DeleteFrameByIndex(int index)
{
	int i = 1;
	auto it = currentPath->begin();
	while (it != currentPath->end())
	{
		if (i == index)
		{
			int frame = it->second.frame;
			currentPath->erase(it);
			cvarManager->log("Deleted snapshot #" + std::to_string(index) + " with ID: " + std::to_string(frame));
			break;
		}
		i++; it++;
	}
	this->RefreshInterpData();
	this->RefreshInterpDataRotation();
}

bool DollyCam::ChangeFrame(const int oldFrame, const int newFrame)
{
	const auto it = currentPath->find(oldFrame);
	if (it != currentPath->end())
	{
		currentPath->erase(it);

		ReplayServerWrapper sw = gameWrapper->GetGameEventAsReplay();
		auto replay = sw.GetReplay();
		replay.SetCurrentFrame(newFrame);
		auto newTimestamp = sw.GetReplayTimeElapsed();

		it->second.frame = newFrame;
		it->second.timeStamp = newTimestamp;
		InsertSnapshot(it->second);

		return true;
	}
	return false;


}

std::vector<int> DollyCam::GetUsedFrames()
{
	std::vector<int> frames = std::vector<int>();
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
inline float Dot(Rotator rot, Vector line)
{
	Vector fov = RotatorToVector(rot);
	return Vector::dot(fov, line);
}
void DollyCam::Render(CanvasWrapper cw)
{
	if (!renderPath || !currentRenderPath || currentRenderPath->size() < 2)
		return;

	ReplayServerWrapper sw = gameWrapper->GetGameEventAsReplay();
	CameraWrapper cam = gameWrapper->GetCamera();
	auto location = cam.GetLocation();
	auto rotation = cam.GetRotation();
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

		line.X = std::max(0, line.X);
		line.X = std::min(line.X, canvasSize.X);
		line.Y = std::max(0, line.Y);
		line.Y = std::min(line.Y, canvasSize.Y);
		bool inFrustum = false;
		Vector cam_to_line = (it->second.location - location);
		cam_to_line.normalize();
		float cam_dot_line = Dot(rotation, cam_to_line);
		if (cam_dot_line > 0) inFrustum = true;
		if (inFrustum) {
			if (!(((line.X < .1 || line.X >= canvasSize.X - .5) && (line.Y < .1 || line.Y >= canvasSize.Y - .5)) || ((prevLine.X < .1 || prevLine.X >= canvasSize.X - .5) && (prevLine.Y < .1 || prevLine.Y >= canvasSize.Y - .5))))
			{
				cw.DrawLine(prevLine, line);
				cw.DrawLine(prevLine.minus({ 1,1 }), line.minus({ 1,1 })); //make lines thicker
				cw.DrawLine(prevLine.minus({ -1,-1 }), line.minus({ -1,-1 }));
				if (renderFrames) {
					cw.SetColor(0, 0, 0, 255);
					cw.SetPosition(line);
					cw.DrawString(std::to_string(it->first));
				}
			}
		}
		prevLine = line;
	}

	int index = 1;
	for (auto it = currentPath->begin(); it != currentPath->end(); it++)
	{
		auto boxLoc = cw.Project(it->second.location);
		bool inFrustum = false;
		Vector cam_to_box = (it->second.location - location);
		cam_to_box.normalize();
		float cam_dot_line = Dot(rotation, cam_to_box);
		if (cam_dot_line > 0) inFrustum = true;
		if (inFrustum) {
			cw.SetColor(255, 0, 0, 255);
			if (boxLoc.X >= 0 && boxLoc.X <= canvasSize.X && boxLoc.Y >= 0 && boxLoc.Y <= canvasSize.Y) {
				boxLoc.X -= 5;
				boxLoc.Y -= 5;
				cw.SetPosition(boxLoc);
				auto tmp = Vector2();
				tmp.X = 10; tmp.Y = 10;
				cw.FillBox(tmp);
				cw.SetColor(255, 255, 255, 255);
				cw.DrawString("(" + std::to_string(index) + ")" + " (ID:" + std::to_string(it->first) + ", w:" + to_string_with_precision(it->second.weight, 2) + ")");
			}
		}
		index++;
	}
}

void DollyCam::RefreshInterpData()
{
	CVarWrapper interpMode = cvarManager->getCvar("dolly_interpmode_location");
	locationInterpStrategy = CreateInterpStrategy(interpMode.getIntValue());
	UpdateRenderPath();
	CheckIfSameInterp();
}

void DollyCam::RefreshInterpDataRotation()
{
	CVarWrapper interpMode = cvarManager->getCvar("dolly_interpmode_rotation");
	rotationInterpStrategy = CreateInterpStrategy(interpMode.getIntValue());
	if (locationInterpStrategy->GetName().compare((rotationInterpStrategy)->GetName()) == 0)
	{
		rotationInterpStrategy = locationInterpStrategy;
	}
	CheckIfSameInterp();
}

std::string DollyCam::GetInterpolationMethod(bool locationInterp)
{
	if (locationInterp && locationInterpStrategy)
		return locationInterpStrategy->GetName();
	else if (rotationInterpStrategy)
		return rotationInterpStrategy->GetName();
	return "none";
}



std::shared_ptr<InterpStrategy> DollyCam::CreateInterpStrategy(int interpStrategy)
{

	int chaikinDegree = cvarManager->getCvar("dolly_chaikin_degree").getIntValue();
	switch (interpStrategy)
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
		std::make_shared<LinearInterpStrategy>(LinearInterpStrategy(currentPath, chaikinDegree));
		//return std::make_shared<HermiteInterpStrategy>(HermiteInterpStrategy(currentPath));
		break;
	case 4:
		return std::make_shared<CatmullRomInterpStrategy>(CatmullRomInterpStrategy(currentPath, chaikinDegree));
		break;
	case 5:
		auto tmp = std::make_shared<SplineInterpStrategy>(SplineInterpStrategy(currentPath, chaikinDegree));
		tmp->cvarManager = cvarManager;
		return tmp;
	}

	cvarManager->log("Interpstrategy not found!!! Defaulting to linear interp.");
	return std::make_shared<LinearInterpStrategy>(LinearInterpStrategy(currentPath, chaikinDegree));
}

void DollyCam::SaveToFile(const std::string& saveDirectory, const std::string& filename)
{
    std::filesystem::path filePath = GetFilePath(saveDirectory, filename);

	std::map<std::string, CameraSnapshot> pathCopy;
	for (auto& i : *currentPath)
	{
		pathCopy.insert_or_assign(std::to_string(i.first), i.second);
	}
	json j = pathCopy;
	std::ofstream myfile(filePath);
	myfile << j.dump(4);
	myfile.close();
}

bool DollyCam::LoadFromFile(const std::string& loadDirectory, const std::string& filename)
{
    std::filesystem::path filePath = GetFilePath(loadDirectory, filename);
    if (!std::filesystem::exists(filePath))
	{
		cvarManager->log("File does not exist!");
		return false;
	}

	std::ifstream i(filePath);
	json j;
	i >> j;
	currentPath->clear();
	auto v8 = j.get<std::map<std::string, CameraSnapshot>>();
	for (auto& i : v8)
	{
		std::string first = i.first;
		int intVal = get_safe_int(first);
		CameraSnapshot value = i.second;
		currentPath->insert_or_assign(intVal, value);
	}

	this->RefreshInterpData();
	this->RefreshInterpDataRotation();

    return true;
}

std::filesystem::path DollyCam::GetFilePath(const std::string& loadDirectory, const std::string& filename)
{
    std::filesystem::path Output = loadDirectory;

    if(loadDirectory.empty())
    {
        Output = gameWrapper->GetDataFolder() / "campaths";
    }

    //Append filename to path using directory separator
    Output /= filename;

    return Output;
}

std::shared_ptr<savetype> DollyCam::GetCurrentPath()
{
	return currentPath;
}

void DollyCam::SetCurrentPath(std::shared_ptr<savetype> newPath)
{
	currentPath = newPath;
}
