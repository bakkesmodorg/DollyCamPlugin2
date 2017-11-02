#include "models.h"

POV NewPOV::ToPOV()
{
	return{ location, rotation.ToRotator(), FOV };
}
