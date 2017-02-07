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

#include <math.h>

#define MIN(a, b) (a) < (b) ? (a) : (b)
#define MAX(a, b) (a) > (b) ? (a) : (b)

struct Vector3f {
	f32 x, y, z;
};

struct Vector4f {
	f32 x, y, z, w;
};

struct Matrix4f {
	Vector4f columns[4];

	static inline Matrix4f identity()
	{
		Matrix4f identity = {};
		identity.columns[0].x = 1.0f;
		identity.columns[1].y = 1.0f;
		identity.columns[2].z = 1.0f;
		identity.columns[3].w = 1.0f;
		return identity;
	}

	static inline Matrix4f orthographic(f32 left, f32 right,
	                                    f32 top, f32 bottom,
	                                    f32 near, f32 far)
	{
		Matrix4f result = Matrix4f::identity();
		result.columns[0].x = 2.0f / (right - left);
		result.columns[1].y = 2.0f / (top - bottom);

		result.columns[2].z = - 2.0f / (far - near);
		result.columns[3].x = - (right + left) / (right - left);
		result.columns[3].y = - (top + bottom ) / (top - bottom);

		result.columns[3].z = (far + near) / (far - near);

		return result;
	}
};

/*******************************************************************************
 * Vector3f function declarations
 ******************************************************************************/
inline f32    length(Vector3f vec);
inline f32    dot(Vector3f lhs, Vector3f rhs);
inline Vector3f cross(Vector3f lhs, Vector3f rhs);

/*******************************************************************************
 * Matrix4f function declarations
 ******************************************************************************/
inline Matrix4f translate(Matrix4f mat, Vector3f v);

/*******************************************************************************
 * Vector3f operator declarations
 ******************************************************************************/
inline Vector3f operator + (Vector3f lhs, Vector3f rhs);
inline Vector3f operator + (Vector3f lhs, f32 rhs);
inline Vector3f operator + (f32 lhs, Vector3f rhs);
inline Vector3f& operator += (Vector3f &lhs, Vector3f rhs);
inline Vector3f& operator += (Vector3f &lhs, f32 rhs);

inline Vector3f operator - (Vector3f lhs, Vector3f rhs);
inline Vector3f operator - (Vector3f lhs, f32 rhs);
inline Vector3f operator - (f32 lhs, Vector3f rhs);
inline Vector3f operator -= (Vector3f lhs, Vector3f rhs);
inline Vector3f operator -= (Vector3f lhs, f32 rhs);

inline Vector3f operator * (Vector3f lhs, f32 rhs);
inline Vector3f operator * (f32 lhs, Vector3f rhs);
inline Vector3f operator *= (Vector3f lhs, f32 rhs);

/*******************************************************************************
 * Vector4f operator declarations
 ******************************************************************************/
inline Vector4f operator + (Vector4f lhs, Vector4f rhs);
inline Vector4f operator + (Vector4f lhs, f32 rhs);
inline Vector4f operator + (f32 lhs, Vector4f rhs);
inline Vector4f& operator += (Vector4f &lhs, Vector4f rhs);
inline Vector4f& operator += (Vector4f &lhs, f32 rhs);

inline Vector4f operator - (Vector4f lhs, Vector4f rhs);
inline Vector4f operator - (Vector4f lhs, f32 rhs);
inline Vector4f operator - (f32 lhs, Vector4f rhs);
inline Vector4f operator -= (Vector4f lhs, Vector4f rhs);
inline Vector4f operator -= (Vector4f lhs, f32 rhs);

inline Vector4f operator * (Vector4f lhs, f32 rhs);
inline Vector4f operator * (f32 lhs, Vector4f rhs);
inline Vector4f operator *= (Vector4f lhs, f32 rhs);

/*******************************************************************************
 * Vector3f function definitions
 ******************************************************************************/
inline f32 length(Vector3f vec)
{
	return std::sqrt(vec.x * vec.x +
	                 vec.y * vec.y +
	                 vec.z * vec.z);
}

inline f32 dot(Vector3f lhs, Vector3f rhs)
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

/******************************************************************************
 * Matrix4f function definitions
 *****************************************************************************/
inline Matrix4f translate(Matrix4f m, Vector3f v)
{
	Matrix4f result = m;
	result.columns[3] = m.columns[0] * v.x +
	                    m.columns[1] * v.y +
	                    m.columns[2] * v.z +
	                    m.columns[3];
	return result;
}



/*******************************************************************************
 * Vector3f operator definitions
 ******************************************************************************/
inline Vector3f operator + (Vector3f lhs, Vector3f rhs)
{
	Vector3f vec;
	vec.x = lhs.x + rhs.x;
	vec.y = lhs.y + rhs.y;
	vec.z = lhs.z + rhs.z;

	return vec;
}

inline Vector3f operator + (Vector3f lhs, f32 rhs)
{
	Vector3f vec;
	vec.x = lhs.x + rhs;
	vec.y = lhs.y + rhs;
	vec.z = lhs.z + rhs;

	return vec;
}

inline Vector3f operator + (f32 lhs, Vector3f rhs)
{
	return rhs + lhs;
}

inline Vector3f& operator += (Vector3f &lhs, Vector3f rhs)
{
	lhs = lhs + rhs;
	return lhs;

}

inline Vector3f& operator += (Vector3f &lhs, f32 rhs)
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

inline Vector3f operator - (Vector3f lhs, f32 rhs)
{
	Vector3f vec;
	vec.x = lhs.x - rhs;
	vec.y = lhs.y - rhs;
	vec.z = lhs.z - rhs;

	return vec;
}

inline Vector3f operator - (f32 lhs, Vector3f rhs)
{
	return rhs - lhs;
}

inline Vector3f & operator -= (Vector3f &lhs, Vector3f rhs)
{
	lhs = lhs - rhs;
	return lhs;
}

inline Vector3f & operator -= (Vector3f &lhs, f32 rhs)
{
	lhs = lhs - rhs;
	return lhs;
}

inline Vector3f operator * (Vector3f lhs, f32 rhs)
{
	Vector3f vec;
	vec.x = lhs.x * rhs;
	vec.y = lhs.y * rhs;
	vec.z = lhs.z * rhs;

	return vec;
}

inline Vector3f operator * (f32 lhs, Vector3f rhs)
{
	return rhs * lhs;
}

inline Vector3f & operator *= (Vector3f &lhs, f32 rhs)
{
	lhs = lhs * rhs;
	return lhs;
}

/*******************************************************************************
 * Vector4f operator definitions
 ******************************************************************************/
inline Vector4f operator + (Vector4f lhs, Vector4f rhs)
{
	Vector4f vec;
	vec.x = lhs.x + rhs.x;
	vec.y = lhs.y + rhs.y;
	vec.z = lhs.z + rhs.z;
	vec.w = lhs.w + rhs.w;

	return vec;
}

inline Vector4f operator + (Vector4f lhs, f32 rhs)
{
	Vector4f vec;
	vec.x = lhs.x + rhs;
	vec.y = lhs.y + rhs;
	vec.z = lhs.z + rhs;
	vec.w = lhs.w + rhs;

	return vec;
}

inline Vector4f  operator + (f32 lhs, Vector4f rhs)
{
	return rhs + lhs;
}

inline Vector4f& operator += (Vector4f &lhs, Vector4f rhs)
{
	lhs = lhs + rhs;
	return lhs;

}

inline Vector4f& operator += (Vector4f &lhs, f32 rhs)
{
	lhs = lhs + rhs;
	return lhs;
}

inline Vector4f operator - (Vector4f lhs, Vector4f rhs)
{
	Vector4f vec;
	vec.x = lhs.x - rhs.x;
	vec.y = lhs.y - rhs.y;
	vec.z = lhs.z - rhs.z;
	vec.w = lhs.w - rhs.w;

	return vec;
}

inline Vector4f operator - (Vector4f lhs, f32 rhs)
{
	Vector4f vec;
	vec.x = lhs.x - rhs;
	vec.y = lhs.y - rhs;
	vec.z = lhs.z - rhs;
	vec.w = lhs.w - rhs;

	return vec;
}

inline Vector4f operator - (f32 lhs, Vector4f rhs)
{
	return rhs - lhs;
}

inline Vector4f & operator -= (Vector4f &lhs, Vector4f rhs)
{
	lhs = lhs - rhs;
	return lhs;
}

inline Vector4f & operator -= (Vector4f &lhs, f32 rhs)
{
	lhs = lhs - rhs;
	return lhs;
}

inline Vector4f operator * (Vector4f lhs, f32 rhs)
{
	Vector4f vec;
	vec.x = lhs.x * rhs;
	vec.y = lhs.y * rhs;
	vec.z = lhs.z * rhs;
	vec.w = lhs.w * rhs;

	return vec;
}

inline Vector4f operator * (f32 lhs, Vector4f rhs)
{
	return rhs * lhs;
}

inline Vector4f & operator *= (Vector4f &lhs, f32 rhs)
{
	lhs = lhs * rhs;
	return lhs;
}

#endif // LEARY_MATH_H
