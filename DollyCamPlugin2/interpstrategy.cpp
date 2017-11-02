#include "interpstrategy.h"

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

	CustomRotator dif = (nextSnapshot->second.rotation - currentSnapshot->second.rotation);
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
