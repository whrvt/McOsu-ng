//================ Copyright (c) 2013, PG, All rights reserved. =================//
//
// Purpose:		a simple quaternion
//
// $NoKeywords: $quat
//===============================================================================//

#pragma once
#ifndef QUATERNION_H
#define QUATERNION_H

#include "cbase.h"

class Quaternion
{
public:
	float x,y,z,w;

	Quaternion() {identity();}
	Quaternion(float x, float y, float z, float w) {this->x = x;this->y = y;this->z = z;this->w = w;}

	void identity() {x = y = z = 0.0f; w = 1.0f;}
	void set(float x, float y, float z, float w);
	void normalize();

	void fromAxis(const Vector3 &axis, float angleDeg);
	void fromEuler(float yawDeg, float pitchDeg, float rollDeg);

	[[nodiscard]] inline float getYaw() const {return glm::degrees(glm::atan2(2.0f*(y*z + w*x), w*w - x*x - y*y + z*z));}
	[[nodiscard]] inline float getPitch() const {return glm::degrees(glm::asin(-2.0f*(x*z - w*y)));}
	[[nodiscard]] inline float getRoll() const {return glm::degrees(glm::atan2(2.0f*(x*y + w*z), w*w + x*x - y*y - z*z));}

	[[nodiscard]] inline Quaternion getConjugate() const {return Quaternion(-x, -y, -z, w);}
	[[nodiscard]] Matrix4 getMatrix() const;
	[[nodiscard]] Matrix3 getMatrix3() const;

	Quaternion operator * (const Quaternion &quat) const;
	Vector3 operator * (const Vector3 &vec) const;

private:
	static constexpr float EPSILON = 0.00001f;
};

#endif
