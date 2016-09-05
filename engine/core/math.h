#ifndef LEARY_MATH_H
#define LEARY_MATH_H

#include <cmath>

struct Vector3f {
	float x, y, z;

	inline Vector3f &operator += (Vector3f rhs);
	inline Vector3f &operator += (float rhs);

	inline Vector3f &operator -= (Vector3f rhs);
	inline Vector3f &operator -= (float rhs);

	inline Vector3f &operator *= (float rhs);
};

inline float 
length(Vector3f vec)
{
	return std::sqrt(vec.x * vec.x + 
	                 vec.y * vec.y + 
	                 vec.z * vec.z);
}

inline float 
dot(Vector3f lhs, Vector3f rhs)
{
	return lhs.x * rhs.x + 
	       lhs.y * rhs.y + 
	       lhs.z * lhs.z;
}

inline Vector3f 
cross(Vector3f lhs, Vector3f rhs)
{
	Vector3f vec;
	vec.x = lhs.y * rhs.z - lhs.z * rhs.y;
	vec.y = lhs.z * rhs.x - lhs.x * rhs.z;
	vec.z = lhs.x * rhs.y - lhs.y * rhs.x;

	return vec;
}

//
//
//

inline Vector3f 
operator + (Vector3f lhs, Vector3f rhs)
{
	Vector3f vec;
	vec.x = lhs.x + rhs.x;
	vec.y = lhs.y + rhs.y;
	vec.z = lhs.z + rhs.z;

	return vec;
}

inline Vector3f 
operator + (Vector3f lhs, float rhs)
{
	Vector3f vec;
	vec.x = lhs.x + rhs;
	vec.y = lhs.y + rhs;
	vec.z = lhs.z + rhs;

	return vec;
}

inline Vector3f 
operator + (float lhs, Vector3f rhs)
{
	return rhs + lhs;
}

inline Vector3f &
Vector3f::operator += (Vector3f rhs)
{
	*this = *this + rhs;
	return *this;
	
}

inline Vector3f &
Vector3f::operator += (float rhs)
{
	*this = *this + rhs;
	return *this;
}

//
//
//

inline Vector3f 
operator - (Vector3f lhs, Vector3f rhs)
{
	Vector3f vec;
	vec.x = lhs.x - rhs.x;
	vec.y = lhs.y - rhs.y;
	vec.z = lhs.z - rhs.z;

	return vec;
}

inline Vector3f 
operator - (Vector3f lhs, float rhs)
{
	Vector3f vec;
	vec.x = lhs.x - rhs;
	vec.y = lhs.y - rhs;
	vec.z = lhs.z - rhs;

	return vec;
}

inline Vector3f 
operator - (float lhs, Vector3f rhs)
{
	return rhs - lhs;
}

inline Vector3f &
Vector3f::operator -= (Vector3f rhs) 
{
	*this = *this - rhs;
	return *this;
}

inline Vector3f &
Vector3f::operator -= (float rhs) 
{
	*this = *this - rhs;
	return *this;
}

//
//
//

inline Vector3f 
operator * (Vector3f lhs, float rhs)
{
	Vector3f vec;
	vec.x = lhs.x * rhs;
	vec.y = lhs.y * rhs;
	vec.z = lhs.z * rhs;

	return vec;
}

inline Vector3f 
operator * (float lhs, Vector3f rhs)
{
	return rhs * lhs;
}

inline Vector3f &
Vector3f::operator *= (float rhs) 
{
	*this  = *this * rhs;
	return *this;
}


#endif // LEARY_MATH_H
