#pragma once
#include <string>
#include "bakkesmod\wrappers\wrapperstructs.h"
#include "nlohmann/json.hpp"
#include "models.h"
std::string vector_to_string(Vector v);

std::string rotator_to_string(Rotator r);

using nlohmann::json;

void to_json(json& j, const Vector& p);

void from_json(const json& j, Vector& p);

void to_json(json& j, const Rotator& p);

void from_json(const json& j, Rotator& p);

void to_json(json& j, const CustomRotator& p);

void from_json(const json& j, CustomRotator& p);

void to_json(json& j, const CameraSnapshot& p);

void from_json(const json& j, CameraSnapshot& p);