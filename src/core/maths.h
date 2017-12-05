/**
 * file:    math.h
 * created: 2017-03-20
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#ifndef MATH_H
#define MATH_H

namespace lry {
    f32 cos(f32 x);
    f32 sin(f32 x);
    f32 tan(f32 x);
    f32 sqrt(f32 x);
    f32 ceil(f32 x);
    f32 floor(f32 x);
    f32 abs(f32 x);
}

// TODO(jesper): better pi(e)
#define PI 3.1415942f
#define F32_MAX 3.402823466e+38F

#define MCOMBINE2(a, b) a ## b
#define MCOMBINE(a, b) MCOMBINE2(a, b)
#define DUMMY_VAR MCOMBINE(dummy_, __LINE__)

union Vector2 {
    f32 data[2];
    struct {
        f32 x, y;
    };
    struct {
        f32 r, g;
    };
    struct {
        f32 u, v;
    };
    struct {
        f32 min, max;
    };
};

union Vector3 {
    f32 data[3];
    struct {
        f32 x, y, z;
    };
    struct {
        f32 r, g, b;
    };

    struct {
        Vector2 xy;
        f32 DUMMY_VAR;
    };
    struct {
        Vector2 rg;
        f32 DUMMY_VAR;
    };

    struct {
        f32 DUMMY_VAR;
        Vector2 yz;
    };
    struct {
        f32 DUMMY_VAR;
        Vector2 gb;
    };
};

union Vector4 {
    f32 data[4];
    struct {
        f32 x, y, z, w;
    };
    struct {
        f32 r, g, b, a;
    };

    struct {
        Vector3 xyz;
        f32 DUMMY_VAR;
    };
    struct {
        Vector3 rgb;
        f32 DUMMY_VAR;
    };

    struct {
        f32 DUMMY_VAR;
        Vector3 yzw;
    };
    struct {
        f32 DUMMY_VAR;
        Vector3 gba;
    };

    struct {
        Vector2 xy;
        Vector2 zw;
    };
    struct {
        Vector2 rg;
        Vector2 ba;
    };

    struct {
        f32 DUMMY_VAR;
        Vector2 yz;
        f32 DUMMY_VAR;
    };
    struct {
        f32 DUMMY_VAR;
        Vector2 gb;
        f32 DUMMY_VAR;
    };
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
		f32 sht = lry::sin(ht);
		f32 cht = lry::cos(ht);

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

		f32 tan_hvfov = lry::tan(vfov / 2.0f);
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


f32
clamp(f32 val, f32 min, f32 max)
{
    val = val > max ? max : val;
    val = val < min ? min : val;
    return val;
}

Vector3
clamp(Vector3 val, Vector3 min, Vector3 max)
{
    val.x = clamp(val.x, min.x, max.x);
    val.y = clamp(val.y, min.y, max.y);
    val.z = clamp(val.z, min.z, max.z);
    return val;
}

#endif /* MATH_H */

