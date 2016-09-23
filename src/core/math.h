/**
 * @file:   math.h
 * @author: Jesper Stefansson (grouse)
 * @email:  jesper.stefansson@gmail.com
 *
 * Copyright (c) 2016 Jesper Stefansson
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgement in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#ifndef LEARY_MATH_H
#define LEARY_MATH_H

#include <cmath>

#define MIN(a, b) (a) < (b) ? (a) : (b)
#define MAX(a, b) (a) > (b) ? (a) : (b)

struct Vector3f {
	float x, y, z;
};

inline float    length(Vector3f vec);
inline float    dot(Vector3f lhs, Vector3f rhs);
inline Vector3f cross(Vector3f lhs, Vector3f rhs);

inline float length(Vector3f vec)
{
	return std::sqrt(vec.x * vec.x + 
	                 vec.y * vec.y + 
	                 vec.z * vec.z);
}

inline float dot(Vector3f lhs, Vector3f rhs)
{
	return lhs.x * rhs.x + 
	       lhs.y * rhs.y + 
	       lhs.z * lhs.z;
}

inline Vector3f cross(Vector3f lhs, Vector3f rhs)
{
	Vector3f vec;
	vec.x = lhs.y * rhs.z - lhs.z * rhs.y;
	vec.y = lhs.z * rhs.x - lhs.x * rhs.z;
	vec.z = lhs.x * rhs.y - lhs.y * rhs.x;

	return vec;
}

/**************************************************************************************************
 * Vector3f operator declarations
 *************************************************************************************************/
inline Vector3f operator + (Vector3f lhs, Vector3f rhs);
inline Vector3f operator + (Vector3f lhs, float rhs);
inline Vector3f operator + (float lhs, Vector3f rhs);
inline Vector3f operator += (Vector3f lhs, Vector3f rhs);
inline Vector3f operator += (Vector3f lhs, float rhs);

inline Vector3f operator - (Vector3f lhs, Vector3f rhs);
inline Vector3f operator - (Vector3f lhs, float rhs);
inline Vector3f operator - (float lhs, Vector3f rhs);
inline Vector3f operator -= (Vector3f lhs, Vector3f rhs);
inline Vector3f operator -= (Vector3f lhs, float rhs);

inline Vector3f operator * (Vector3f lhs, float rhs);
inline Vector3f operator * (float lhs, Vector3f rhs);
inline Vector3f operator *= (Vector3f lhs, float rhs);

/**************************************************************************************************
 * Vector3f operator declarations
 *************************************************************************************************/
inline Vector3f operator + (Vector3f lhs, Vector3f rhs)
{
	Vector3f vec;
	vec.x = lhs.x + rhs.x;
	vec.y = lhs.y + rhs.y;
	vec.z = lhs.z + rhs.z;

	return vec;
}

inline Vector3f operator + (Vector3f lhs, float rhs)
{
	Vector3f vec;
	vec.x = lhs.x + rhs;
	vec.y = lhs.y + rhs;
	vec.z = lhs.z + rhs;

	return vec;
}

inline Vector3f operator + (float lhs, Vector3f rhs)
{
	return rhs + lhs;
}

inline Vector3f & operator += (Vector3f &lhs, Vector3f rhs)
{
	lhs = lhs + rhs;
	return lhs;
	
}

inline Vector3f & operator += (Vector3f &lhs, float rhs)
{
	lhs = lhs + rhs;
	return lhs;
}

inline Vector3f operator - (Vector3f lhs, Vector3f rhs)
{
	Vector3f vec;
	vec.x = lhs.x - rhs.x;
	vec.y = lhs.y - rhs.y;
	vec.z = lhs.z - rhs.z;

	return vec;
}

inline Vector3f operator - (Vector3f lhs, float rhs)
{
	Vector3f vec;
	vec.x = lhs.x - rhs;
	vec.y = lhs.y - rhs;
	vec.z = lhs.z - rhs;

	return vec;
}

inline Vector3f operator - (float lhs, Vector3f rhs)
{
	return rhs - lhs;
}

inline Vector3f & operator -= (Vector3f &lhs, Vector3f rhs) 
{
	lhs = lhs - rhs;
	return lhs;
}

inline Vector3f & operator -= (Vector3f &lhs, float rhs) 
{
	lhs = lhs - rhs;
	return lhs;
}

inline Vector3f operator * (Vector3f lhs, float rhs)
{
	Vector3f vec;
	vec.x = lhs.x * rhs;
	vec.y = lhs.y * rhs;
	vec.z = lhs.z * rhs;

	return vec;
}

inline Vector3f operator * (float lhs, Vector3f rhs)
{
	return rhs * lhs;
}

inline Vector3f & operator *= (Vector3f &lhs, float rhs) 
{
	lhs = lhs * rhs;
	return lhs;
}


#endif // LEARY_MATH_H
