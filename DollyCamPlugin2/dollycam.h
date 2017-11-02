#pragma once
#include <memory>
#include <map>
#include "utils\customrotator.h"
#include "bakkesmod\plugin\bakkesmodplugin.h"
#include "gameapplier.h"
#include "models.h"
#include "interpstrategy.h"

class DollyCam
{
private:
	std::shared_ptr<savetype> currentPath;
	std::shared_ptr<GameWrapper> gameWrapper;
	std::shared_ptr<CVarManagerWrapper> cvarManager;
	std::shared_ptr<IGameApplier> gameApplier;
	std::shared_ptr<InterpStrategy> interpStrategy;
	bool isActive = false;
public:
	DollyCam(std::shared_ptr<GameWrapper> _gameWrapper, std::shared_ptr<CVarManagerWrapper> _cvarManager, std::shared_ptr<IGameApplier> _gameApplier);
	~DollyCam();

	//Takes a snapshot of the current camera state and adds it to current path, returns true if taking snapshot was sucsessfull
	bool TakeSnapshot();
	bool IsActive();
	void Activate();
	void Deactivate();
	void Apply();
};

