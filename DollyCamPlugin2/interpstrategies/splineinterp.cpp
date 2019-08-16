#include <map>
#include "splineinterp.h"
#include "nbezierinterp.h"
//#include "bakkesmod\wrappers\wrapperstructs.h"

vector<tinyspline::real> SolveForT(tinyspline::BSpline &spline, float tGoal, float e, int maxSteps = 50)
{
	float uMin = 0;
	float uMax = 1;

	double u = (uMin + uMax) / 2;
	auto res = spline.eval(u).result();
	auto t = res[0];
	float error = abs(t - tGoal);
	int steps = 1;
	while (error > e || steps > maxSteps)
	{
		if (t < tGoal) {
			uMin = (uMin + uMax) / 2;
		}
		if (t > tGoal)
		{
			uMax = (uMin + uMax) / 2;
		}
		u = (uMin + uMax) / 2;
		res = spline.eval(u).result();
		t = res[0];
		error = abs(t - tGoal);
		steps++;
	}
	return res;
}


SplineInterpStrategy::SplineInterpStrategy(std::shared_ptr<savetype> _camPath, int degree)
{
	setCamPath(_camPath, degree);
	backupStrategy = std::make_shared<NBezierInterpStrategy>(NBezierInterpStrategy(_camPath, degree));
}

NewPOV SplineInterpStrategy::GetPOV(float gameTime, int latestFrame)
{
	//auto t = GetRelativeTime(gameTime);
	auto t = GetRelativeTimeFromFrame(latestFrame);


	int n = camPath->size();
	if (n < 4)
	{
		return backupStrategy->GetPOV(gameTime, latestFrame);
	}
	auto nextSnapshot = camPath->upper_bound(latestFrame);
	auto currentSnapshot = std::prev(nextSnapshot);
	if (currentSnapshot == camPath->end() || nextSnapshot == camPath->end() || camPath->begin()->first > latestFrame || t > 1) //We're at the end of the playback
		return{ Vector(0), CustomRotator(0,0,0), 0 };

	//Could this be done in the constructor?
	InitPositions(n);
	InitRotations(n);
	InitFOVs(n);

	float epsilon = 0.001; // Acceptable error is 1 / 1000 seconds.
	auto posRes = camPositions.bisect(gameTime, epsilon).result();
	auto rotRes = camRotations.bisect(gameTime, epsilon).result();
	auto fovRes = camFOVs.bisect(gameTime, epsilon).result();


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

float SplineInterpStrategy::GetRelativeTimeFromFrame(int frame)
{
	auto startSnapshot = camPath->begin();
	auto endSnapshot = (--camPath->end());

	float startFrame = startSnapshot->second.frame;
	float endFrame = endSnapshot->second.frame;

	float totalFrames = endFrame - startFrame;
	float relativeTime = (frame - startFrame) / totalFrames;
	if (relativeTime < 0) return 0;
	if (relativeTime > 1) return 1;
	return relativeTime;
}


void SplineInterpStrategy::InitFOVs(int numberOfPoints)
{
	auto POVs = vector<tinyspline::real>();

	auto fovsControllPoints = camFOVs.controlPoints();
	for (const auto& item : *camPath)
	{
		auto point = item.second;
		POVs.push_back(double(point.timeStamp));
		POVs.push_back(double(point.FOV));
	}
	camFOVs = tinyspline::Utils::interpolateCubic(&POVs, 2);
}

void SplineInterpStrategy::InitRotations(int numberOfPoints)
{
	//(t, x, y, z)
	auto rotations = vector<tinyspline::real>();

	auto previousRotation = camPath->begin()->second.rotation;
	float accumulatedPitch = previousRotation.Pitch._value;
	float accumulatedYaw = previousRotation.Yaw._value;
	float accumulatedRoll = previousRotation.Roll._value;

	for (const auto& item : *camPath)
	{
		auto point = item.second;
		auto thisRotator = point.rotation;
		auto diffRotation = previousRotation.diffTo(thisRotator);

		accumulatedPitch += diffRotation.Pitch._value;
		accumulatedYaw += diffRotation.Yaw._value;
		accumulatedRoll += diffRotation.Roll._value;

		previousRotation = thisRotator;

		rotations.push_back(double(point.timeStamp));
		rotations.push_back(double(accumulatedPitch));
		rotations.push_back(double(accumulatedYaw));
		rotations.push_back(double(accumulatedRoll));
	}

	camRotations = tinyspline::Utils::interpolateCubic(&rotations, 4);
}

void SplineInterpStrategy::InitPositions(int numberOfPoints)
{
	//(t, x, y, z)
	auto positions = vector<tinyspline::real>();
	for (const auto& item : *camPath)
	{
		auto point = item.second;
		positions.push_back(double(point.timeStamp));
		positions.push_back(double(point.location.X));
		positions.push_back(double(point.location.Y));
		positions.push_back(double(point.location.Z));
	}
	camPositions = tinyspline::Utils::interpolateCubic(&positions, 4);
}
