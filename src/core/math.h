/**
 * file:    math.h
 * created: 2017-03-20
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#ifndef MATH_H
#define MATH_H

// TODO(jesper): better pi(e)
#define PI 3.1415942f

INTROSPECT struct Vector2 {
	f32 x, y;
};

INTROSPECT struct Vector3 {
	f32 x, y, z;
};

INTROSPECT struct Vector4 {
	f32 x, y, z, w;

	void foo() {
	}
};

struct Quaternion {
	f32 x, y, z, w;

	static inline Quaternion make(Vector3 v)
	{
		Quaternion q;
		q = { v.x, v.y, v.z, 0.0f };
		return q;
	}

	static inline Quaternion make(Vector3 a, f32 theta)
	{
		Quaternion q;

		f32 ht  = theta / 2.0f;
		f32 sht = sinf(ht);
		f32 cht = cosf(ht);

		q.x = a.x * sht;
		q.y = a.y * sht;
		q.z = a.z * sht;
		q.w = cht;

		return q;
	}

	static inline Quaternion yaw(f32 theta)
	{
		return Quaternion::make({ 0.0f, 1.0f, 0.0f }, theta);
	}

	static inline Quaternion pitch(f32 theta)
	{
		return Quaternion::make({ 0.0f, 0.0f, 1.0f }, theta);
	}
};

INTROSPECT struct Matrix4 {
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

		f32 tan_hvfov = tanf(vfov / 2.0f);
		result[0].x = 1.0f / (aspect * tan_hvfov);
		result[1].y = 1.0f / (tan_hvfov);
		result[2].w = -1.0f;
		result[2].z = far / (near - far);
		result[3].z = -(far * near) / (far - near);
		return result;
	}

	static inline Matrix4 make(Quaternion q)
	{
		Matrix4 r;

		f32 qxx = q.x * q.x;
		f32 qyy = q.y * q.y;
		f32 qzz = q.z * q.z;
		f32 qxz = q.x * q.z;
		f32 qxy = q.x * q.y;
		f32 qyz = q.y * q.z;
		f32 qwx = q.w * q.x;
		f32 qwy = q.w * q.y;
		f32 qwz = q.w * q.z;

		r[0].x = 1.0f - 2.0f * (qyy +  qzz);
		r[0].y = 2.0f * (qxy + qwz);
		r[0].z = 2.0f * (qxz - qwy);
		r[0].w = 0.0f;

		r[1].x = 2.0f * (qxy - qwz);
		r[1].y = 1.0f - 2.0f * (qxx +  qzz);
		r[1].z = 2.0f * (qyz + qwx);
		r[1].w = 0.0f;

		r[2].x = 2.0f * (qxz + qwy);
		r[2].y = 2.0f * (qyz - qwx);
		r[2].z = 1.0f - 2.0f * (qxx +  qyy);
		r[2].w = 0.0f;

		r[3] = { 0.0f, 0.0f, 0.0f, 1.0f };

		return r;
	}
};

#endif /* MATH_H */

