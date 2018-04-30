#include "nbezierinterp.h"



NBezierInterpStrategy::NBezierInterpStrategy(std::shared_ptr<savetype> _camPath, int degree)
{
	setCamPath(_camPath, degree);
}

uint64_t calc_factorial(uint64_t n) //cache this maybe
{
	return (n == 1 || n == 0) ? 1 : calc_factorial(n - 1) * n;
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
		Rotator r2 = { (int)((float)usedRotator.Pitch * pofact), (int)((float)usedRotator.Yaw * pofact), (int)((float)usedRotator.Roll * pofact) };
		lastRotator = usedRotator;
		v = v + v2;
		rot.Pitch += r2.Pitch;
		rot.Yaw += r2.Yaw;
		rot.Roll += r2.Roll;

		fov = fov + fov2;
		k++;
	}
	return{ v, rot, fov };
}

std::string NBezierInterpStrategy::GetName()
{
	return "nth bezier interpolation";
}