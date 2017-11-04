#include "serialization.h"
#include "utils\parser.h"
#include "bakkesmod\wrappers\wrapperstructs.h"

std::string vector_to_string(Vector v)
{
	return to_string_with_precision(v.X, 2) + ", " + to_string_with_precision(v.Y, 2) + ", " + to_string_with_precision(v.Z, 2);
}

std::string rotator_to_string(Rotator r)
{
	return to_string_with_precision(r.Pitch, 2) + ", " + to_string_with_precision(r.Yaw, 2) + ", " + to_string_with_precision(r.Roll, 2);
}
