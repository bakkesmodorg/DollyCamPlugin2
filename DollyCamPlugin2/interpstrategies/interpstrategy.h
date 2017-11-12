#pragma once
#include "../models.h"
#include <memory>
#include <map>
#include <string>
class InterpStrategy
{
protected:
	std::unique_ptr<savetype> camPath;
	void setCamPath(std::shared_ptr<savetype> path, int chaikinAmount);
public:

	virtual NewPOV GetPOV(float gameTime, int latestFrame) = 0;
	virtual std::string GetName() = 0;
};





class CosineInterpStrategy : public InterpStrategy
{
public:
	CosineInterpStrategy(std::shared_ptr<savetype> _camPath);
	virtual NewPOV GetPOV(float gameTime, int latestFrame);
	virtual std::string GetName();
};

class HermiteInterpStrategy : public InterpStrategy
{
public:
	HermiteInterpStrategy(std::shared_ptr<savetype> _camPath);
	virtual NewPOV GetPOV(float gameTime, int latestFrame);
	virtual std::string GetName();
};

class CatmullRomInterpStrategy : public InterpStrategy
{
public:
	CatmullRomInterpStrategy(std::shared_ptr<savetype> _camPath);
	virtual NewPOV GetPOV(float gameTime, int latestFrame);
	virtual std::string GetName();
};