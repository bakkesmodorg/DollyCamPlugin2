#pragma once
#include "interpstrategy.h"
#include "tinyspline\tinysplinecpp.h"


class SplineInterpStrategy : public InterpStrategy
{
public:
	SplineInterpStrategy(std::shared_ptr<savetype> _camPath, int degree);
	virtual NewPOV GetPOV(float gameTime, int latestFrame);
	virtual std::string GetName();

private:
	float GetRelativeTime(float gameTime);

	void InitFOVs(int numberOfPoints);
	void InitRotations(int numberOfPoints);
	void InitPositions(int numberOfPoints);

	tinyspline::BSpline camPositions;
	tinyspline::BSpline camRotations;
	tinyspline::BSpline camFOVs;

	std::shared_ptr<InterpStrategy> backupStrategy;
};