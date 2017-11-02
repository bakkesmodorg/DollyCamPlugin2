#pragma once
#include "models.h"
#include <memory>
#include <map>
class InterpStrategy
{
protected:
	std::unique_ptr<savetype> camPath;
public:

	virtual NewPOV GetPOV(float gameTime, int latestFrame) = 0;
};

class LinearInterpStrategy : public InterpStrategy
{
public:
	LinearInterpStrategy(std::shared_ptr<savetype> _camPath);
	virtual NewPOV GetPOV(float gameTime, int latestFrame);
};