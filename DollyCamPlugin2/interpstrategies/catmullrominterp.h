#pragma once
#include "interpstrategy.h"
#include "linearinterp.h"
class CatmullRomInterpStrategy : public InterpStrategy
{
private:
	std::shared_ptr<LinearInterpStrategy> linearInterp;
public:
	CatmullRomInterpStrategy(std::shared_ptr<savetype> _camPath, int chaikinDegree);
	virtual NewPOV GetPOV(float gameTime, int latestFrame);
	virtual std::string GetName();
};