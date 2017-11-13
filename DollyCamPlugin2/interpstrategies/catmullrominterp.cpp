#include "catmullrominterp.h"



CatmullRomInterpStrategy::CatmullRomInterpStrategy(std::shared_ptr<savetype> _camPath, int chaikinDegree)
{
	setCamPath(_camPath, chaikinDegree);
	linearInterp = std::make_shared<LinearInterpStrategy>(LinearInterpStrategy(_camPath, chaikinDegree));
}

float GetCatmullRomPosition(float t, float p0, float p1, float p2, float p3)
{
	return 0.5f * ((2 * p1) +
		(p2 - p0) * t +
		(2 * p0 - 5 * p1 + 4 * p2 - p3) * t * t +
		(3 * p1 - p0 - 3 * p2 + p3) * t * t * t);
	////The coefficients of the cubic polynomial (except the 0.5f * which I added later for performance)
	//float a = 2.f * p1;
	//float b = p2 - p0;
	//float c = 2.f * p0 - 5.f * p1 + 4.f * p2 - p3;
	//float d = -p0 + 3.f * p1 - 3.f * p2 + p3;

	////The cubic polynomial: a + b * t + c * t^2 + d * t^3
	//float pos = 0.5f * (a + (b * t) + (c * t * t) + (d * t * t * t));

	//return pos;
}

Vector catmullRom(float t, Vector p0, Vector p1, Vector p2, Vector p3)
{
	return Vector(GetCatmullRomPosition(t, p0.X, p1.X, p2.X, p3.X),
		GetCatmullRomPosition(t, p0.Y, p1.Y, p2.Y, p3.Y),
		GetCatmullRomPosition(t, p0.Z, p1.Z, p2.Z, p3.Z));
	//Vector a = p1 * Vector(2.f);
	//Vector b = p2 - p0;
	//Vector c = p0  * Vector(2.f) - p1 * Vector(5.f) +  p2 * Vector(4.f) - p3;
	//Vector p0neg = Vector(-p0.X, -p0.Y, -p0.Z);
	//Vector d = p0neg + Vector(3.f) * p1 - Vector(3.f) * p2 + p3;

	////The cubic polynomial: a + b * t + c * t^2 + d * t^3
	//Vector pos = Vector(0.5f) * (a + (b * t) + (c * t * t) + (d * t * t * t));

	//return pos;
}
CustomRotator catmullRom(float t, CustomRotator p0, CustomRotator p1, CustomRotator p2, CustomRotator p3)
{
	return CustomRotator(GetCatmullRomPosition(t, p0.Pitch._value, p1.Pitch._value, p2.Pitch._value, p3.Pitch._value),
		GetCatmullRomPosition(t, p0.Yaw._value, p1.Yaw._value, p2.Yaw._value, p3.Yaw._value),
		GetCatmullRomPosition(t, p0.Roll._value, p1.Roll._value, p2.Roll._value, p3.Roll._value));
	//Vector a = p1 * Vector(2.f);
	//Vector b = p2 - p0;
	//Vector c = p0  * Vector(2.f) - p1 * Vector(5.f) +  p2 * Vector(4.f) - p3;
	//Vector p0neg = Vector(-p0.X, -p0.Y, -p0.Z);
	//Vector d = p0neg + Vector(3.f) * p1 - Vector(3.f) * p2 + p3;

	////The cubic polynomial: a + b * t + c * t^2 + d * t^3
	//Vector pos = Vector(0.5f) * (a + (b * t) + (c * t * t) + (d * t * t * t));

	//return pos;
}


NewPOV CatmullRomInterpStrategy::GetPOV(float gameTime, int latestFrame)
{
	if (camPath->size() < 4) //Need atleast 4 elements
		return{ 0 };
	//gameTime -= camPath->begin()->second.timeStamp;
	//bool isFirst = false;
	//bool isLast = false;
	auto startSnapshot = camPath->upper_bound(latestFrame);
	if (startSnapshot->first == camPath->begin()->first)
	{
		return linearInterp->GetPOV(gameTime, latestFrame);
	}
	if (startSnapshot->first == (--camPath->end())->first || startSnapshot->first == camPath->end()->first)
	{
		return linearInterp->GetPOV(gameTime, latestFrame);
	}

	int goBack = 2;

	if (startSnapshot == camPath->end())
		goBack = 4; //3 if islast
	else if (startSnapshot == (--camPath->end()))
		goBack = 3; //2 if islast

	for (int i = 0; i < goBack && startSnapshot != camPath->begin(); i++) //Go to first snapshot needed for hermite
	{
		startSnapshot = std::prev(startSnapshot);
	}

	auto currentSnapshot = /*isFirst ? startSnapshot : */std::next(startSnapshot);
	auto nextSnapshot = /*isLast ? currentSnapshot : */std::next(currentSnapshot);
	auto nextNextSnapshot = /*isLast ? nextSnapshot :*/ std::next(nextSnapshot);

	float totalDiff = nextSnapshot->second.timeStamp - currentSnapshot->second.timeStamp;// nextNextSnapshot->second.timeStamp - startSnapshot->second.timeStamp;
	float percElapsed = (gameTime - currentSnapshot->second.timeStamp) / totalDiff;

	NewPOV newPov;
	newPov.location = catmullRom(percElapsed, startSnapshot->second.location, currentSnapshot->second.location, nextSnapshot->second.location, nextNextSnapshot->second.location);
	newPov.rotation = catmullRom(percElapsed, startSnapshot->second.rotation, currentSnapshot->second.rotation, nextSnapshot->second.rotation, nextNextSnapshot->second.rotation);
	newPov.FOV = GetCatmullRomPosition(percElapsed, startSnapshot->second.FOV, currentSnapshot->second.FOV, nextSnapshot->second.FOV, nextNextSnapshot->second.FOV);
	//newPov.FOV = 90;
	return newPov;
}

std::string CatmullRomInterpStrategy::GetName()
{
	return "Catmull-Rom interpolation";
}