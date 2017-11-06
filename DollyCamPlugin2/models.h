#pragma once
#include "utils\customrotator.h"
#define savetype std::map<int, CameraSnapshot>

struct NewPOV
{
	Vector location;
	CustomRotator rotation;
	float FOV;
	POV ToPOV();
};

struct CameraSnapshot
{
	//int id; 
	int frame = 0.f; //Lets use frames as ID's
	float timeStamp = 0.f;
	float FOV = 0.f;
	Vector location;
	CustomRotator rotation;

	float weight = 1.f;
};