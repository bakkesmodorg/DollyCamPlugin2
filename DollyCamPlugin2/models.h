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
	int frame; //Lets use frames as ID's
	float timeStamp;
	float FOV;
	Vector location;
	CustomRotator rotation;
};