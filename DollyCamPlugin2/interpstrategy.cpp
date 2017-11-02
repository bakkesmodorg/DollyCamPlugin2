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

	NewPOV pov;
	pov.location = currentSnapshot->second.location + ((nextSnapshot->second.location - currentSnapshot->second.location) * timeElapsed) / snap;
	pov.rotation = currentSnapshot->second.rotation + ((currentSnapshot->second.rotation.diffTo(nextSnapshot->second.rotation)) * percElapsed) / snapR;
	pov.FOV = currentSnapshot->second.FOV + ((nextSnapshot->second.FOV - currentSnapshot->second.FOV) * percElapsed);
	
	return pov;
}
