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


void to_json(json& j, const Vector& p) {
	j = json{ { "x", p.X },{ "y", p.Y },{ "z", p.Z } };
}

void from_json(const json& j, Vector& p) {
	p.X = j.at("x").get<float>();
	p.Y = j.at("y").get<float>();
	p.Z = j.at("z").get<float>();
}

void to_json(json& j, const Rotator& p) {
	j = json{ { "pitch", p.Pitch },{ "yaw", p.Yaw },{ "roll", p.Roll } };
}

void from_json(const json& j, Rotator& p) {
	p.Pitch = j.at("pitch").get<float>();
	p.Yaw = j.at("yaw").get<float>();
	p.Roll = j.at("roll").get<float>();
}

void to_json(json& j, const CustomRotator& p) {
	j = json{ { "pitch", p.Pitch._value },{ "yaw", p.Yaw._value },{ "roll", p.Roll._value } };
}

void from_json(const json& j, CustomRotator& p) {
	p.Pitch._value = j.at("pitch").get<float>();
	p.Yaw._value = j.at("yaw").get<float>();
	p.Roll._value = j.at("roll").get<float>();
}

void to_json(json& j, const CameraSnapshot& p) {
	j = json{ { "frame", p.frame },{ "timestamp", p.timeStamp },{ "FOV", p.FOV },
	{ "location", p.location },{ "rotation", p.rotation },{ "weight", p.weight } };
}

void from_json(const json& j, CameraSnapshot& p) {
	p.frame = j.at("frame").get<int>();
	p.timeStamp = j.at("timestamp").get<float>();
	p.FOV = j.at("FOV").get<float>();
	p.location = j.at("location").get<Vector>();
	p.rotation = (j.at("rotation").get<CustomRotator>());
	p.weight = j.at("weight").get<float>();
}
