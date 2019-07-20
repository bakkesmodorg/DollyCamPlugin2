#include <map>
#include "splineinterp.h"
#include "tinyspline\tinysplinecpp.h"
#include "nbezierinterp.h"
#include "bakkesmod\wrappers\wrapperstructs.h"
#include "Quaternions\quaternions.h"



SplineInterpStrategy::SplineInterpStrategy(std::shared_ptr<savetype> _camPath, int degree)
{
	setCamPath(_camPath, degree);
	backupStrategy = std::make_shared<NBezierInterpStrategy>(NBezierInterpStrategy(_camPath, degree));
}

NewPOV SplineInterpStrategy::GetPOV(float gameTime, int latestFrame)
{
	auto t = GetRelativeTime(gameTime);

	CustomRotator rot;
	Vector v;
	float fov = 0;
	int n = camPath->size();
	if (n < 4)
	{
		return backupStrategy->GetPOV(gameTime, latestFrame);
	}
	auto nextSnapshot = camPath->upper_bound(latestFrame);
	auto currentSnapshot = std::prev(nextSnapshot);
	if (currentSnapshot == camPath->end() || nextSnapshot == camPath->end() || camPath->begin()->first > latestFrame) //We're at the end of the playback
		return{ Vector(0), CustomRotator(0,0,0), 0 };

	InitPositions(n);
	//InitRotations(n);
	InitFOVs(n);

	auto posRes = camPositions.eval(t).result();
	//auto rotRes = camRotations.eval(t).result();
	auto fovRes = camFOVs.eval(t).result();

	v.X = float(posRes[1]);
	v.Y = float(posRes[2]);
	v.Z = float(posRes[3]);

	fov = float(fovRes[1]);


	float frameDiff = nextSnapshot->second.timeStamp - currentSnapshot->second.timeStamp;
	float timeElapsed = gameTime - currentSnapshot->second.timeStamp;
	float percElapsed = timeElapsed / frameDiff;

	auto prevRot = currentSnapshot->second.rotation;
	auto nextRot = nextSnapshot->second.rotation;

	auto q1 = ToQuaternion(prevRot);
	auto q2 = ToQuaternion(nextRot);
	auto qSlerp = slerp(q1, q2, percElapsed);
	rot = ToCustomRotator(qSlerp);

	//auto prevVec = RotatorToVector(prevRot.ToRotator());
	//auto nextVec = RotatorToVector(nextRot.ToRotator());
	//auto slerpVec = Vector::slerp(prevVec, nextVec, percElapsed);
	//rot = CustomRotator(VectorToRotator(slerpVec));

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

	auto rotationControllPoints = camRotations.controlPoints();
	rotationControllPoints.clear();

	for (const auto& item : *camPath)
	{
		auto point = item.second;
		rotationControllPoints.push_back(double(point.timeStamp));
		rotationControllPoints.push_back(double(point.rotation.Pitch));
		rotationControllPoints.push_back(double(point.rotation.Yaw));
		rotationControllPoints.push_back(double(point.rotation.Roll));
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
