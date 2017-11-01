#pragma once
#include "bakkesmod\wrappers\gamewrapper.h"
#include "utils\customrotator.h"
#include <iostream>
#include <fstream>
#include "models.h"

//Interface for applying stuff to the game which will help with simulations
class IGameApplier
{
public:
	virtual void SetPOV(Vector location, CustomRotator rotation, float FOV) = 0;
	virtual NewPOV GetPOV() = 0;
};

//Applies the given angles to the Rocket League game
class RealGameApplier : public IGameApplier {
private:
	std::shared_ptr<GameWrapper> gameWrapper;
public:
	RealGameApplier(std::shared_ptr<GameWrapper> gw);
	void SetPOV(Vector location, CustomRotator rotation, float FOV);
	NewPOV GetPOV();
};

//Mock game applier for simulating data
class MockGameApplier : public IGameApplier {
private:
	ofstream output;
	NewPOV pov;
	float time;
public:
	MockGameApplier(string filename);
	~MockGameApplier();
	void SetTime(float time);
	void SetPOV(Vector location, CustomRotator rotation, float FOV);
	NewPOV GetPOV();
};