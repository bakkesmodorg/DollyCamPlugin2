#pragma once
#include "utils\customrotator.h"
#include "bakkesmod\wrappers\wrapperstructs.h"
#include <math.h>

constexpr auto M_PI = 3.1415f;

struct Quaternion
{
	float w, x, y, z;

	void normalize()
	{
		float d = sqrtf((w * w + x * x + y * y + z * z));
		w = w / d;
		x = x / d;
		y = y / d;
		z = z / d;
	}

	Quaternion operator-() const {
		return { -w, -x, -y, -z };
	}

	Quaternion operator*(float f) const {
		return { w*f, x*f, y*f, z*f };
	}

};

inline 	Quaternion operator-(const Quaternion& v1, const Quaternion& v2) {
	return { v1.w - v2.w, v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
}

inline 	Quaternion operator+(const Quaternion& v1, const Quaternion& v2) {
	//return { v1.w + v2.w, v1.x + v2.x, v1.y + v2.y, v1.z + v1.z }; //nast bug be nasty
	return { v1.w + v2.w, v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
}

Quaternion ToQuaternion(float pitch, float yaw, float roll) // yaw (Z), pitch (Y), roll (X)
{
	// Abbreviations for the various angular functions
	float cy = cos(yaw * 0.5f);
	float sy = sin(yaw * 0.5f);
	float cp = cos(pitch * 0.5f);
	float sp = sin(pitch * 0.5f);
	float cr = cos(roll * 0.5f);
	float sr = sin(roll * 0.5f);

	Quaternion q;
	q.w = cy * cp * cr + sy * sp * sr;
	q.x = cy * cp * sr - sy * sp * cr;
	q.y = sy * cp * sr + cy * sp * cr;
	q.z = sy * cp * cr - cy * sp * sr;

	return q;
}

Quaternion ToQuaternion(CustomRotator rot)
{
	return ToQuaternion(rot.Pitch._value * float(CONST_UnrRotToRad), rot.Yaw._value * float(CONST_UnrRotToRad), rot.Roll._value * float(CONST_UnrRotToRad));
}

CustomRotator ToCustomRotator(Quaternion q)
{

	// roll (x-axis rotation)
	float sinr_cosp = +2.0f * (q.w * q.x + q.y * q.z);
	float cosr_cosp = +1.0f - 2.0f * (q.x * q.x + q.y * q.y);
	float roll = atan2(sinr_cosp, cosr_cosp);

	// pitch (y-axis rotation)
	float sinp = +2.0f * (q.w * q.y - q.z * q.x);
	float pitch;
	if (fabs(sinp) >= 1)
		pitch = copysign(M_PI / 2, sinp); // use 90 degrees if out of range
	else
		pitch = asin(sinp);

	// yaw (z-axis rotation)
	float siny_cosp = +2.0f * (q.w * q.z + q.x * q.y);
	float cosy_cosp = +1.0f - 2.0f * (q.y * q.y + q.z * q.z);
	float yaw = atan2(siny_cosp, cosy_cosp);


	return CustomRotator(pitch * float(CONST_RadToUnrRot), yaw * float(CONST_RadToUnrRot), roll * float(CONST_RadToUnrRot));
}

float dot_product(Quaternion a, Quaternion b)
{
	return  a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

Quaternion lerp(Quaternion v0, Quaternion v1, float t)
{
	v0.normalize();
	v1.normalize();
	auto res = v0 + (v1 - v0)*t;
	res.normalize();
	return res;
}

Quaternion slerp(Quaternion v0, Quaternion v1, float t) {
	// Only unit quaternions are valid rotations.
	// Normalize to avoid undefined behavior.
	v0.normalize();
	v1.normalize();

	// Compute the cosine of the angle between the two vectors.
	float dot = dot_product(v0, v1);

	// If the dot product is negative, slerp won't take
	// the shorter path. Note that v1 and -v1 are equivalent when
	// the negation is applied to all four components. Fix by 
	// reversing one quaternion.
	if (dot < 0.0f) {
		v1 = -v1;
		dot = -dot;
	}

	const float DOT_THRESHOLD = 0.9995f;
	if (dot > DOT_THRESHOLD) {
		// If the inputs are too close for comfort, linearly interpolate
		// and normalize the result.

		Quaternion result = v0 + (v1 - v0)*t;
		result.normalize();
		return result;
	}

	// Since dot is in range [0, DOT_THRESHOLD], acos is safe
	float theta_0 = acos(dot);        // theta_0 = angle between input vectors
	float theta = theta_0 * t;          // theta = angle between v0 and result
	float sin_theta = sin(theta);     // compute this value only once
	float sin_theta_0 = sin(theta_0); // compute this value only once

	float s0 = cos(theta) - dot * sin_theta / sin_theta_0;  // == sin(theta_0 - theta) / sin(theta_0)
	float s1 = sin_theta / sin_theta_0;

	auto res = (v0 * s0) + (v1 * s1);
	res.normalize();
	return res;
}