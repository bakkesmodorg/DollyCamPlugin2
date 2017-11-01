#pragma once
#include "models.h"
#include <memory>
#include <map>
class InterpStrategy
{
private:
	std::unique_ptr<savetype> camPath;
public:
	InterpStrategy(std::unique_ptr<savetype> _camPath);
	~InterpStrategy();

	virtual NewPOV GetPOV(float gameTime, int latestFrame);
};

