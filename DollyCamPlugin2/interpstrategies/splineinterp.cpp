#include <map>
#include "splineinterp.h"
#include "nbezierinterp.h"
//#include "bakkesmod\wrappers\wrapperstructs.h"



SplineInterpStrategy::SplineInterpStrategy(std::shared_ptr<savetype> _camPath, int degree)
{
	setCamPath(_camPath, degree);
	backupStrategy = std::make_shared<NBezierInterpStrategy>(NBezierInterpStrategy(_camPath, degree));
}

NewPOV SplineInterpStrategy::GetPOV(float gameTime, int latestFrame)
{
	auto t = GetRelativeTime(gameTime);


	int n = camPath->size();
	if (n < 4)
	{
		return backupStrategy->GetPOV(gameTime, latestFrame);
	}
	auto nextSnapshot = camPath->upper_bound(latestFrame);
	auto currentSnapshot = std::prev(nextSnapshot);
	if (currentSnapshot == camPath->end() || nextSnapshot == camPath->end() || camPath->begin()->first > latestFrame || t > 1) //We're at the end of the playback
		return{ Vector(0), CustomRotator(0,0,0), 0 };

	InitPositions(n);
	InitRotations(n);
	InitFOVs(n);

	auto posRes = camPositions.eval(t).result();
	auto rotRes = camRotations.eval(t).result();
	auto fovRes = camFOVs.eval(t).result();

	Vector v;
	v.X = float(posRes[1]);
	v.Y = float(posRes[2]);
	v.Z = float(posRes[3]);

	float fov = float(fovRes[1]);

	CustomRotator rot = CustomRotator(float(rotRes[1]), float(rotRes[2]), float(rotRes[3]));

	return {v, rot, fov};
}

std::string SplineInterpStrategy::GetName()
{
	return "Spline interpolation";
}

float SplineInterpStrategy::GetRelativeTime(float gameTime)
{
	auto startSnapshot = camPath->begin();
	auto endSnapshot = (--camPath->end());

	float totalTime = endSnapshot->second.timeStamp - startSnapshot->second.timeStamp;
	gameTime -= startSnapshot->second.timeStamp;
	float t = gameTime / totalTime;
	return t;
}


void SplineInterpStrategy::InitFOVs(int numberOfPoints)
{
	camFOVs = tinyspline::BSpline(numberOfPoints, 2);

	auto fovsControllPoints = camFOVs.controlPoints();
	fovsControllPoints.clear();
	for (const auto& item : *camPath)
	{
		auto point = item.second;
		fovsControllPoints.push_back(double(point.timeStamp));
		fovsControllPoints.push_back(double(point.FOV));
	}
	camFOVs.setControlPoints(fovsControllPoints);
}

void SplineInterpStrategy::InitRotations(int numberOfPoints)
{
	camRotations = tinyspline::BSpline(numberOfPoints, 4);

	auto previousRotation = camPath->begin()->second.rotation;
	float accumulatedPitch = previousRotation.Pitch._value;
	float accumulatedYaw = previousRotation.Yaw._value;
	float accumulatedRoll = previousRotation.Roll._value;

	auto rotationControllPoints = camRotations.controlPoints();
	rotationControllPoints.clear();

	for (const auto& item : *camPath)
	{
		auto point = item.second;
		auto thisRotator = point.rotation;
		auto diffRotation = previousRotation.diffTo(thisRotator);

		accumulatedPitch += diffRotation.Pitch._value;
		accumulatedYaw += diffRotation.Yaw._value;
		accumulatedRoll += diffRotation.Roll._value;

		previousRotation = thisRotator;

		rotationControllPoints.push_back(double(point.timeStamp));
		rotationControllPoints.push_back(double(accumulatedPitch));
		rotationControllPoints.push_back(double(accumulatedYaw));
		rotationControllPoints.push_back(double(accumulatedRoll));
	}
	camRotations.setControlPoints(rotationControllPoints);
}

void SplineInterpStrategy::InitPositions(int numberOfPoints)
{
	//(t, x, y, z)
	camPositions = tinyspline::BSpline(numberOfPoints, 4);

	auto positions = camPositions.controlPoints();
	positions.clear();

	for (const auto& item : *camPath)
	{
		auto point = item.second;
		positions.push_back(double(point.timeStamp));
		positions.push_back(double(point.location.X));
		positions.push_back(double(point.location.Y));
		positions.push_back(double(point.location.Z));
	}
	camPositions.setControlPoints(positions);
}
