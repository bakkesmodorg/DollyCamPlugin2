#pragma once
#include "utils\customrotator.h"
#define savetype std::map<float, CameraSnapshot>

struct NewPOV
{
	Vector location;
	CustomRotator rotation;
	float FOV;
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