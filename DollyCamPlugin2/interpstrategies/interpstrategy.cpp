#include "interpstrategy.h"

#define M_PI           3.14159265358979323846

CosineInterpStrategy::CosineInterpStrategy(std::shared_ptr<savetype> _camPath)
{
	camPath = std::make_unique<savetype>(*_camPath);
}

NewPOV CosineInterpStrategy::GetPOV(float gameTime, int latestFrame)
{
	auto nextSnapshot = camPath->upper_bound(latestFrame);
	auto currentSnapshot = std::prev(nextSnapshot);
	// std::next(currentSnapshot);
	if (currentSnapshot == camPath->end() || nextSnapshot == camPath->end() || camPath->begin()->first > latestFrame) //We're at the end of the playback
		return{ Vector(0), CustomRotator(0,0,0), 0 };


	float frameDiff = nextSnapshot->second.timeStamp - currentSnapshot->second.timeStamp;
	float timeElapsed = gameTime - currentSnapshot->second.timeStamp;
	float percElapsed = timeElapsed / frameDiff;

	float t2 = (1 - cos(percElapsed*M_PI)) / 2;
	NewPOV newPov;
	newPov.location = currentSnapshot->second.location*(1 - t2) + nextSnapshot->second.location*t2;
	newPov.rotation = currentSnapshot->second.rotation*(1 - t2) + nextSnapshot->second.rotation*t2;
	newPov.FOV = currentSnapshot->second.FOV*(1 - t2) + nextSnapshot->second.FOV*t2;
	return newPov;
}

std::string CosineInterpStrategy::GetName()
{
	return "cosine interpolation";
}

HermiteInterpStrategy::HermiteInterpStrategy(std::shared_ptr<savetype> _camPath)
{
	camPath = std::make_unique<savetype>(*_camPath);
}

//Vector hermiteVector(Vector y0, Vector y1, Vector y2, Vector y3, float totalDiff)
//{
//	Vector m0, m1, mu2, mu3;
//	Vector a0, a1, a2, a3;
//	Vector bias = Vector(0);
//	Vector tension = Vector(0);
//
//	mu2 = Vector(totalDiff * totalDiff);
//	mu3 = mu2 * totalDiff;
//
//	//m0 =  ;
//	Vector one = (y1 - y0);
//	auto test2 = 1;
//	auto test = (Vector(1) + bias);
//	Vector two = one*test;
//	Vector three = two*(Vector(1) - tension);
//	Vector four = three / 2;
//	m0 = four;
//	m0 = m0 + ((y2 - y1)*(Vector(1) - bias)*(1 - tension) / 2);
//	m1 = (y2 - y1)*(1 + bias)*(1 - tension) / 2;
//	m1 = m1 + ((y3 - y2)*(1 - bias)*(1 - tension) / 2);
//	a0 = mu3 * 2 - mu2 * 3 + 1;
//	a1 = mu3 - mu2 * 2 + totalDiff;
//	a2 = mu3 - mu2;
//	a3 = mu3 * -2 + mu2 * 3;
//
//	return(a0*y1 + a1*m0 + a2*m1 + a3*y2);
//}



template<typename T>
T test(T y0, T y1, T y2, T y3, float totalDiff)
{
	T m0, m1, mu2, mu3;
	T a0, a1, a2, a3;
	T bias = T(0);
	T tension = T(0);

	mu2 = T(totalDiff * totalDiff);
	mu3 = mu2 * totalDiff;

	//m0 =  ;
	T one = (y1 - y0);
	T two = one*(T(1) + bias);
	T three = two*(T(1) - tension);
	T four = three / 2;
	m0 = four;
	m0 = m0 + ((y2 - y1)*(T(1) - bias)*(T(1) - tension) / 2);
	m1 = (y2 - y1)*(T(1) + bias)*(T(1) - tension) / 2;
	m1 = m1 + ((y3 - y2)*(T(1) - bias)*(T(1) - tension) / 2);
	a0 = mu3 * 2 - mu2 *3 + 1;
	a1 = mu3 - mu2 * 2 + totalDiff;
	a2 = mu3 - mu2;
	a3 = mu3 * -2 + mu2 * 3;

	return(a0*y1 + a1*m0 + a2*m1 + a3*y2);
}

float hermiteInterp(float y0, float y1, float y2, float y3, float t)
{
	float m0, m1, mu2, mu3;
	float a0, a1, a2, a3;
	float bias = 0;
	float tension = 0;

	mu2 = t * t;
	mu3 = mu2 * t;
	m0 = (y1 - y0)*(1 + bias)*(1 - tension) / 2;
	m0 += (y2 - y1)*(1 - bias)*(1 - tension) / 2;
	m1 = (y2 - y1)*(1 + bias)*(1 - tension) / 2;
	m1 += (y3 - y2)*(1 - bias)*(1 - tension) / 2;
	a0 = 2 * mu3 - 3 * mu2 + 1;
	a1 = mu3 - 2 * mu2 + t;
	a2 = mu3 - mu2;
	a3 = -2 * mu3 + 3 * mu2;

	return(a0*y1 + a1*m0 + a2*m1 + a3*y2);
}


Vector hermiteInterp(Vector p0, Vector p1, Vector p2, Vector p3, float t)
{
	return Vector(hermiteInterp(p0.X, p1.X, p2.X, p3.X, t), hermiteInterp(p0.Y, p1.Y, p2.Y, p3.Y, t), hermiteInterp(p0.Z, p1.Z, p2.Z, p3.Z, t));
}

CustomRotator hermiteInterp(CustomRotator p0, CustomRotator p1, CustomRotator p2, CustomRotator p3, float t)
{
	return CustomRotator(hermiteInterp((float)p0.Pitch, (float)p1.Pitch, (float)p2.Pitch, (float)p3.Pitch, t), hermiteInterp((float)p0.Yaw, (float)p1.Yaw, (float)p2.Yaw, (float)p3.Yaw, t), hermiteInterp((float)p0.Roll, (float)p1.Roll, (float)p2.Roll, (float)p3.Roll, t));
}

NewPOV HermiteInterpStrategy::GetPOV(float gameTime, int latestFrame)
{
	if (camPath->size() < 4) //Need atleast 4 elements
		return{ 0 };
	//gameTime -= camPath->begin()->second.timeStamp;
	auto startSnapshot = camPath->upper_bound(latestFrame);
	int goBack = 2;

	if (startSnapshot == camPath->end())
		goBack = 4;
	else if (startSnapshot == (--camPath->end()))
		goBack = 3;

	for (int i = 0; i < goBack && startSnapshot != camPath->begin(); i++) //Go to first snapshot needed for hermite
	{
		startSnapshot = std::prev(startSnapshot);
	}

	auto currentSnapshot = std::next(startSnapshot);
	auto nextSnapshot = std::next(currentSnapshot);
	auto nextNextSnapshot = std::next(nextSnapshot);

	float totalDiff = nextSnapshot->second.timeStamp - currentSnapshot->second.timeStamp;// nextNextSnapshot->second.timeStamp - startSnapshot->second.timeStamp;
	float percElapsed = gameTime / totalDiff;

	NewPOV newPov;
	newPov.location = hermiteInterp(startSnapshot->second.location, currentSnapshot->second.location, nextSnapshot->second.location, nextNextSnapshot->second.location, percElapsed);
	newPov.rotation = hermiteInterp(startSnapshot->second.rotation, currentSnapshot->second.rotation, nextSnapshot->second.rotation, nextNextSnapshot->second.rotation, percElapsed);
	newPov.FOV = hermiteInterp(startSnapshot->second.FOV, currentSnapshot->second.FOV, nextSnapshot->second.FOV, nextNextSnapshot->second.FOV, percElapsed);
	return newPov;
}

std::string HermiteInterpStrategy::GetName()
{
	return "hermite interpolation";
}

CatmullRomInterpStrategy::CatmullRomInterpStrategy(std::shared_ptr<savetype> _camPath)
{
	camPath = std::make_unique<savetype>(*_camPath);
}

float GetCatmullRomPosition(float t, float p0, float p1, float p2, float p3)
{
	//The coefficients of the cubic polynomial (except the 0.5f * which I added later for performance)
	float a = 2.f * p1;
	float b = p2 - p0;
	float c = 2.f * p0 - 5.f * p1 + 4.f * p2 - p3;
	float d = -p0 + 3.f * p1 - 3.f * p2 + p3;

	//The cubic polynomial: a + b * t + c * t^2 + d * t^3
	float pos = 0.5f * (a + (b * t) + (c * t * t) + (d * t * t * t));

	return pos;
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

NewPOV CatmullRomInterpStrategy::GetPOV(float gameTime, int latestFrame)
{
	if (camPath->size() < 4) //Need atleast 4 elements
		return{ 0 };
	//gameTime -= camPath->begin()->second.timeStamp;
	auto startSnapshot = camPath->upper_bound(latestFrame);
	int goBack = 2;

	if (startSnapshot == camPath->end())
		goBack = 4;
	else if (startSnapshot == (--camPath->end()))
		goBack = 3;

	for (int i = 0; i < goBack && startSnapshot != camPath->begin(); i++) //Go to first snapshot needed for hermite
	{
		startSnapshot = std::prev(startSnapshot);
	}

	auto currentSnapshot = std::next(startSnapshot);
	auto nextSnapshot = std::next(currentSnapshot);
	auto nextNextSnapshot = std::next(nextSnapshot);

	float totalDiff = nextNextSnapshot->second.timeStamp - startSnapshot->second.timeStamp;// nextNextSnapshot->second.timeStamp - startSnapshot->second.timeStamp;
	float percElapsed = gameTime / totalDiff;

	NewPOV newPov;
	newPov.location = catmullRom(percElapsed, startSnapshot->second.location, currentSnapshot->second.location, nextSnapshot->second.location, nextNextSnapshot->second.location);
	/*newPov.rotation = catmullRom(startSnapshot->second.rotation, currentSnapshot->second.rotation, nextSnapshot->second.rotation, nextNextSnapshot->second.rotation, percElapsed);
	newPov.FOV = catmullRom(startSnapshot->second.FOV, currentSnapshot->second.FOV, nextSnapshot->second.FOV, nextNextSnapshot->second.FOV, percElapsed);*/
	newPov.FOV = 90;
	return newPov;
}

std::string CatmullRomInterpStrategy::GetName()
{
	return "Catmull-Rom interpolation";
}

void InterpStrategy::setCamPath(std::shared_ptr<savetype> _camPath, int chaikinAmount)
{
	camPath = std::make_unique<savetype>(*_camPath);
	
	for (int i = 0; i < chaikinAmount; i++) {
		savetype inbetweenPath = savetype();
		for (auto it = camPath->begin(); it != (--camPath->end()); it++)
		{
			CameraSnapshot current = it->second;
			CameraSnapshot next = std::next(it)->second;

			CameraSnapshot p25;
			p25.frame = current.frame * .75f + next.frame * .25f;
			p25.FOV = current.FOV * .75f + next.FOV * .25f;
			p25.location = current.location * .75f + next.location * .25f;
			p25.rotation = current.rotation * .75f + next.rotation * .25f;
			p25.timeStamp = current.timeStamp * .75f + next.timeStamp * .25f;
			p25.weight = current.weight * .75f + next.weight * .25f;

			CameraSnapshot p75;
			p75.frame = current.frame * .25f + next.frame * .75f;
			p75.FOV = current.FOV * .25f + next.FOV * .75f;
			p75.location = current.location * .25f + next.location * .75f;
			p75.rotation = current.rotation * .25f + next.rotation * .75f;
			p75.timeStamp = current.timeStamp * .25f + next.timeStamp * .75f;
			p75.weight = current.weight * .25f + next.weight * .75f;

			inbetweenPath.insert(std::make_pair(p25.frame, p25));
			inbetweenPath.insert(std::make_pair(p75.frame, p75));
		}
		camPath->insert(inbetweenPath.begin(), inbetweenPath.end());
	}
}
