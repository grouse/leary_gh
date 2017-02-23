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

#define PI 3.1415942

#define MIN(a, b) (a) < (b) ? (a) : (b)
#define MAX(a, b) (a) > (b) ? (a) : (b)

INTROSPECT struct Vector3 {
	f32 x, y, z;
};

INTROSPECT struct Vector4 {
	f32 x, y, z, w;
};

struct Matrix4 {
	Vector4 columns[4];

	inline Vector4& operator[] (i32 i)
	{
		return columns[i];
	}

	static inline Matrix4 identity()
	{
		Matrix4 identity = {};
		identity[0].x = 1.0f;
		identity[1].y = 1.0f;
		identity[2].z = 1.0f;
		identity[3].w = 1.0f;
		return identity;
	}

	static inline Matrix4 orthographic(f32 left, f32 right,
	                                    f32 top, f32 bottom,
	                                    f32 near, f32 far)
	{
		Matrix4 result = Matrix4::identity();
		result[0].x = 2.0f / (right - left);
		result[1].y = 2.0f / (top - bottom);
		result[2].z = - 2.0f / (far - near);
		result[3].x = - (right + left) / (right - left);
		result[3].y = - (top + bottom ) / (top - bottom);
		result[3].z = (far + near) / (far - near);
		return result;
	}

	static inline Matrix4 perspective(f32 vfov, f32 aspect, f32 near, f32 far)
	{
		Matrix4 result = {};

		f32 tan_hvfov = tan(vfov / 2.0f);
		result[0].x = 1.0f / (aspect * tan_hvfov);
		result[1].y = 1.0f / (tan_hvfov);
		result[2].w = -1.0f;
		result[2].z = far / (near - far);
		result[3].z = -(far * near) / (far - near);
		return result;
	}
};


inline constexpr f32 radians(f32 degrees)
{
	return degrees * PI / 180;
}


/*******************************************************************************
 * Vector3 function declarations
 ******************************************************************************/
inline f32      length    (Vector3 vec);
inline f32      dot       (Vector3 lhs, Vector3 rhs);
inline Vector3 cross     (Vector3 lhs, Vector3 rhs);
inline Vector3 normalise (Vector3 v);

/*******************************************************************************
 * Vector3 operator declarations
 ******************************************************************************/
inline Vector3  operator +  (Vector3 lhs,  Vector3 rhs);
inline Vector3  operator +  (Vector3 lhs,  f32      rhs);
inline Vector3  operator +  (f32      lhs,  Vector3 rhs);
inline Vector3& operator += (Vector3 &lhs, Vector3 rhs);
inline Vector3& operator += (Vector3 &lhs, f32      rhs);

inline Vector3  operator -  (Vector3 lhs,  Vector3 rhs);
inline Vector3  operator -  (Vector3 lhs,  f32      rhs);
inline Vector3  operator -  (f32      lhs,  Vector3 rhs);
inline Vector3& operator -= (Vector3 &lhs, Vector3 rhs);
inline Vector3& operator -= (Vector3 &lhs, f32      rhs);

inline Vector3  operator *  (Vector3 lhs,  f32      rhs);
inline Vector3  operator *  (f32      lhs,  Vector3 rhs);
inline Vector3& operator *= (Vector3 &lhs, f32      rhs);

/*******************************************************************************
 * Vector4 operator declarations
 ******************************************************************************/
inline Vector4  operator +  (Vector4 lhs,  Vector4 rhs);
inline Vector4  operator +  (Vector4 lhs,  f32      rhs);
inline Vector4  operator +  (f32      lhs,  Vector4 rhs);
inline Vector4& operator += (Vector4 &lhs, Vector4 rhs);
inline Vector4& operator += (Vector4 &lhs, f32      rhs);

inline Vector4  operator -  (Vector4 lhs,  Vector4 rhs);
inline Vector4  operator -  (Vector4 lhs,  f32      rhs);
inline Vector4  operator -  (f32      lhs,  Vector4 rhs);
inline Vector4& operator -= (Vector4 &lhs, Vector4 rhs);
inline Vector4& operator -= (Vector4 &lhs, f32      rhs);

inline Vector4  operator *  (Vector4 lhs,  f32      rhs);
inline Vector4  operator *  (f32      lhs,  Vector4 rhs);
inline Vector4& operator *= (Vector4 &lhs, f32      rhs);

/*******************************************************************************
 * Matrix4 function declarations
 ******************************************************************************/
inline Matrix4 translate(Matrix4 mat, Vector3 v);
inline Matrix4 rotate(Matrix4 mat, f32 angle, Vector3 axis);
inline Matrix4 rotate_x(Matrix4 mat, f32 angle);
inline Matrix4 rotate_y(Matrix4 mat, f32 angle);
inline Matrix4 rotate_z(Matrix4 mat, f32 angle);
inline Matrix4 look_at(Vector3 eye, Vector3 origin, Vector3 up);

/*******************************************************************************
 * Matrix4 operator declarations
 ******************************************************************************/
inline Vector3 operator * (Matrix4 lhs, Vector3 rhs);


/*******************************************************************************
 * Vector3 function definitions
 ******************************************************************************/
inline f32 length(Vector3 vec)
{
	return std::sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
}

inline f32 dot(Vector3 lhs, Vector3 rhs)
{
	return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * lhs.z;
}

inline Vector3 cross(Vector3 lhs, Vector3 rhs)
{
	Vector3 vec;
	vec.x = lhs.y * rhs.z - lhs.z * rhs.y;
	vec.y = lhs.z * rhs.x - lhs.x * rhs.z;
	vec.z = lhs.x * rhs.y - lhs.y * rhs.x;
	return vec;
}

inline Vector3 normalise(Vector3 v)
{
	f32 l = length(v);

	Vector3 result;
	result.x = v.x / l;
	result.y = v.y / l;
	result.z = v.z / l;
	return result;
}

/******************************************************************************
 * Matrix4 function definitions
 *****************************************************************************/
inline Matrix4 translate(Matrix4 m, Vector3 v)
{
	Matrix4 result = m;
	result[3].x += v.x;
	result[3].y += v.y;
	result[3].z += v.z;
	return result;
}

inline Matrix4 rotate(Matrix4 mat, f32 angle, Vector3 v)
{
	f32 c = cos(angle);
	f32 s = sin(angle);

	Vector3 axis = normalise(v);
	Vector3 tmp = (1.0f - c) * axis;

	Matrix4 rmat;
	rmat[0].x = c + tmp.x * axis.x;
	rmat[0].y = tmp.x * axis.y + s * axis.z;
	rmat[0].z = tmp.x * axis.z - s * axis.y;

	rmat[1].x = tmp.y * axis.x - s * axis.z;
	rmat[1].y = c + tmp.y * axis.y;
	rmat[1].z = tmp.y * axis.z + s * axis.x;

	rmat[2].x = tmp.z * axis.x + s * axis.y;
	rmat[2].y = tmp.z * axis.y - s * axis.x;
	rmat[2].z = c + tmp.z * axis.z;

	Matrix4 result;
	result[0] = mat[0] * rmat[0].x + mat[1] * rmat[0].y + mat[2] * rmat[0].z;
	result[1] = mat[0] * rmat[1].x + mat[1] * rmat[1].y + mat[2] * rmat[1].z;
	result[2] = mat[0] * rmat[2].x + mat[1] * rmat[2].y + mat[2] * rmat[2].z;
	result[3] = mat[3];
	return result;
}

inline Matrix4 rotate_x(Matrix4 mat, f32 angle)
{
	// TODO(jesper): optimise these as the axis is known
	return rotate(mat, angle, {1.0f, 0.0f, 0.0f});
}

inline Matrix4 rotate_y(Matrix4 mat, f32 angle)
{
	// TODO(jesper): optimise these as the axis is known
	return rotate(mat, angle, {0.0f, 1.0f, 0.0f});
}

inline Matrix4 rotate_z(Matrix4 mat, f32 angle)
{
	// TODO(jesper): optimise these as the axis is known
	return rotate(mat, angle, {0.0f, 0.0f, 1.0f});
}

inline Matrix4 look_at(Vector3 eye, Vector3 origin, Vector3 up)
{
	Vector3 f = normalise(origin - eye);
	Vector3 s = normalise(cross(f, up));
	Vector3 u = cross(s, f);

	Matrix4 result = Matrix4::identity();
	result[0].x = s.x;
	result[1].x = s.y;
	result[2].x = s.z;
	result[0].y = u.x;
	result[1].y = u.y;
	result[2].y = u.z;
	result[0].z = -f.x;
	result[1].z = -f.y;
	result[2].z = -f.z;
	result[3].x = -dot(s, eye);
	result[3].y = -dot(u, eye);
	result[3].z =  dot(f, eye);
	return result;
}

/*******************************************************************************
 * Matrix4 operator definitions
 ******************************************************************************/
inline Vector3 operator * (Matrix4 lhs, Vector3 rhs)
{
	Vector3 result = {};
	result.x = lhs[0].x * rhs.x + lhs[0].y * rhs.y + lhs[0].z * rhs.z + lhs[3].x;
	result.y = lhs[1].x * rhs.x + lhs[1].y * rhs.y + lhs[1].z * rhs.z + lhs[3].y;
	result.z = lhs[2].x * rhs.x + lhs[2].y * rhs.y + lhs[2].z * rhs.z + lhs[3].z;
	return result;
}


/*******************************************************************************
 * Vector3 operator definitions
 ******************************************************************************/
inline Vector3 operator + (Vector3 lhs, Vector3 rhs)
{
	Vector3 vec;
	vec.x = lhs.x + rhs.x;
	vec.y = lhs.y + rhs.y;
	vec.z = lhs.z + rhs.z;
	return vec;
}

inline Vector3 operator + (Vector3 lhs, f32 rhs)
{
	Vector3 vec;
	vec.x = lhs.x + rhs;
	vec.y = lhs.y + rhs;
	vec.z = lhs.z + rhs;
	return vec;
}

inline Vector3 operator + (f32 lhs, Vector3 rhs)
{
	return rhs + lhs;
}

inline Vector3& operator += (Vector3 &lhs, Vector3 rhs)
{
	lhs = lhs + rhs;
	return lhs;

}

inline Vector3& operator += (Vector3 &lhs, f32 rhs)
{
	lhs = lhs + rhs;
	return lhs;
}

inline Vector3 operator - (Vector3 lhs, Vector3 rhs)
{
	Vector3 vec;
	vec.x = lhs.x - rhs.x;
	vec.y = lhs.y - rhs.y;
	vec.z = lhs.z - rhs.z;
	return vec;
}

inline Vector3 operator - (Vector3 lhs, f32 rhs)
{
	Vector3 vec;
	vec.x = lhs.x - rhs;
	vec.y = lhs.y - rhs;
	vec.z = lhs.z - rhs;
	return vec;
}

inline Vector3 operator - (f32 lhs, Vector3 rhs)
{
	return rhs - lhs;
}

inline Vector3& operator -= (Vector3 &lhs, Vector3 rhs)
{
	lhs = lhs - rhs;
	return lhs;
}

inline Vector3& operator -= (Vector3 &lhs, f32 rhs)
{
	lhs = lhs - rhs;
	return lhs;
}

inline Vector3 operator * (Vector3 lhs, f32 rhs)
{
	Vector3 vec;
	vec.x = lhs.x * rhs;
	vec.y = lhs.y * rhs;
	vec.z = lhs.z * rhs;
	return vec;
}

inline Vector3 operator * (f32 lhs, Vector3 rhs)
{
	return rhs * lhs;
}

inline Vector3& operator *= (Vector3 &lhs, f32 rhs)
{
	lhs = lhs * rhs;
	return lhs;
}

/*******************************************************************************
 * Vector4 operator definitions
 ******************************************************************************/
inline Vector4 operator + (Vector4 lhs, Vector4 rhs)
{
	Vector4 vec;
	vec.x = lhs.x + rhs.x;
	vec.y = lhs.y + rhs.y;
	vec.z = lhs.z + rhs.z;
	vec.w = lhs.w + rhs.w;
	return vec;
}

inline Vector4 operator + (Vector4 lhs, f32 rhs)
{
	Vector4 vec;
	vec.x = lhs.x + rhs;
	vec.y = lhs.y + rhs;
	vec.z = lhs.z + rhs;
	vec.w = lhs.w + rhs;
	return vec;
}

inline Vector4  operator + (f32 lhs, Vector4 rhs)
{
	return rhs + lhs;
}

inline Vector4& operator += (Vector4 &lhs, Vector4 rhs)
{
	lhs = lhs + rhs;
	return lhs;
}

inline Vector4& operator += (Vector4 &lhs, f32 rhs)
{
	lhs = lhs + rhs;
	return lhs;
}

inline Vector4 operator - (Vector4 lhs, Vector4 rhs)
{
	Vector4 vec;
	vec.x = lhs.x - rhs.x;
	vec.y = lhs.y - rhs.y;
	vec.z = lhs.z - rhs.z;
	vec.w = lhs.w - rhs.w;
	return vec;
}

inline Vector4 operator - (Vector4 lhs, f32 rhs)
{
	Vector4 vec;
	vec.x = lhs.x - rhs;
	vec.y = lhs.y - rhs;
	vec.z = lhs.z - rhs;
	vec.w = lhs.w - rhs;
	return vec;
}

inline Vector4 operator - (f32 lhs, Vector4 rhs)
{
	return rhs - lhs;
}

inline Vector4& operator -= (Vector4 &lhs, Vector4 rhs)
{
	lhs = lhs - rhs;
	return lhs;
}

inline Vector4& operator -= (Vector4 &lhs, f32 rhs)
{
	lhs = lhs - rhs;
	return lhs;
}

inline Vector4 operator * (Vector4 lhs, f32 rhs)
{
	Vector4 vec;
	vec.x = lhs.x * rhs;
	vec.y = lhs.y * rhs;
	vec.z = lhs.z * rhs;
	vec.w = lhs.w * rhs;
	return vec;
}

inline Vector4 operator * (f32 lhs, Vector4 rhs)
{
	return rhs * lhs;
}

inline Vector4& operator *= (Vector4 &lhs, f32 rhs)
{
	lhs = lhs * rhs;
	return lhs;
}

#endif // LEARY_MATH_H
