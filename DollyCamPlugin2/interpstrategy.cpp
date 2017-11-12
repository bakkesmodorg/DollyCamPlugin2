#include "interpstrategy.h"

#define M_PI           3.14159265358979323846

LinearInterpStrategy::LinearInterpStrategy(std::shared_ptr<savetype> _camPath)
{
	camPath = std::make_unique<savetype>(*_camPath); //Copy campath
}

NewPOV LinearInterpStrategy::GetPOV(float gameTime, int latestFrame)
{
	auto nextSnapshot = camPath->upper_bound(latestFrame);
	auto currentSnapshot = std::prev(nextSnapshot);
	 // std::next(currentSnapshot);
	if (currentSnapshot == camPath->end() || nextSnapshot == camPath->end() || camPath->begin()->first > latestFrame) //We're at the end of the playback
		return{ Vector(0), CustomRotator(0,0,0), 0 };
	

	float frameDiff = nextSnapshot->second.timeStamp - currentSnapshot->second.timeStamp;
	float timeElapsed = gameTime - currentSnapshot->second.timeStamp;
	float percElapsed = timeElapsed / frameDiff;

	CustomRotator snapR = CustomRotator(frameDiff, frameDiff, frameDiff);
	Vector snap = Vector(frameDiff);

	NewPOV pov; //((currentSnapshot->second.rotation.diffTo(nextSnapshot->second.rotation))
	pov.location = currentSnapshot->second.location + (((nextSnapshot->second.location - currentSnapshot->second.location) * timeElapsed)/snap);

	CustomRotator dif = (currentSnapshot->second.rotation.diffTo(nextSnapshot->second.rotation));
	CustomRotator dif2 = dif * percElapsed;
	CustomRotator rot2 = currentSnapshot->second.rotation + dif2;
	pov.rotation = rot2;
	//FiniteElement<float> pitchDif = (nextSnapshot->second.rotation.Pitch - currentSnapshot->second.rotation.Pitch);
	//FiniteElement<float> pitchDif2 = (pitchDif * percElapsed);
	//pov.rotation.Pitch = currentSnapshot->second.rotation.Pitch + pitchDif2;



	//pov.rotation.Yaw = currentSnapshot->second.rotation.Yaw + ((nextSnapshot->second.rotation.Yaw - currentSnapshot->second.rotation.Yaw) * percElapsed);
	//pov.rotation.Roll = currentSnapshot->second.rotation.Roll + ((nextSnapshot->second.rotation.Roll - currentSnapshot->second.rotation.Roll) * percElapsed);
	//

	pov.FOV = currentSnapshot->second.FOV + (((nextSnapshot->second.FOV - currentSnapshot->second.FOV) * timeElapsed) / frameDiff);
	
	return pov;
}

std::string LinearInterpStrategy::GetName()
{
	return "linear interpolation";
}

NBezierInterpStrategy::NBezierInterpStrategy(std::shared_ptr<savetype> _camPath)
{
	camPath = std::make_unique<savetype>(*_camPath);
	rotInterp = std::make_unique<LinearInterpStrategy>(LinearInterpStrategy(_camPath));
}

uint64_t factorial(int n)
{
	return (n == 1 || n == 0) ? 1 : factorial(n - 1) * n;
}

NewPOV NBezierInterpStrategy::GetPOV(float gameTime, int latestFrame)
{
	auto startSnapshot = camPath->begin();
	auto endSnapshot = (--camPath->end());

	float totalTime = endSnapshot->second.timeStamp - startSnapshot->second.timeStamp;
	gameTime -= startSnapshot->second.timeStamp;
	float t = gameTime / totalTime;

	CustomRotator rot;
	Vector v;
	float fov = 0;
	int n = camPath->size();
	int k = 0;
	Rotator lastRotator = camPath->begin()->second.rotation.ToRotator();
	Rotator addedRotator = { -16364 + 16340, -32768 + 32764, -32768 + 32764 };
	for (savetype::iterator it = camPath->begin(); it != camPath->end(); it++)
	{
		float weight = it->second.weight;


		uint64_t fact = (factorial(n - 1) / ((factorial(k)*factorial((n - 1) - k)))); //Maximum of 19 points =\  
		float po = pow(1 - t, n - 1 - k) * pow(t, k);
		float pofact = po * fact;

//		float weight = it->second.weight;
		//if (it != l2->cbegin() && it != --(l2->cend())) {
		//	//weight = (++it)->first - (--it)->first;
		//}
		//pofact *= weight;

		Vector v2 = Vector(pofact) * it->second.location;
		
		
		float fov2 = pofact * (weight * it->second.FOV);
		Rotator usedRotator;
		if (it != camPath->begin()) {
			Rotator rotDiff = CustomRotator(lastRotator).diffTo(it->second.rotation).ToRotator();
			usedRotator = lastRotator + rotDiff;
		}
		else
		{
			usedRotator = it->second.rotation.ToRotator();
		}
		Rotator r2 = { usedRotator.Pitch * pofact, usedRotator.Yaw * pofact, usedRotator.Roll * pofact };
		lastRotator = usedRotator;
		v = v + v2;
		rot.Pitch += r2.Pitch;
		rot.Yaw += r2.Yaw;
		rot.Roll += r2.Roll;
		//rot += r2;

		fov = fov + fov2;
		k++;
	}
	//CustomRotator newRot = rotInterp->GetPOV(gameTime, latestFrame).rotation;
	
	//rot.Pitch._value = newRot.Pitch._value;
	//rot.Yaw._value = newRot.Yaw._value;
	//rot.Roll._value = newRot.Roll._value;
	return{ v, rot, fov };
}

std::string NBezierInterpStrategy::GetName()
{
	return "nth bezier interpolation";
}

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
