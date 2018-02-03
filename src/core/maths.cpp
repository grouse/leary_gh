/**
 * @file:   math.cpp
 * @author: Jesper Stefansson (grouse)
 * @email:  jesper.stefansson@gmail.com
 *
 * Copyright (c) 2016-2017 Jesper Stefansson
 */

#include "maths.h"
#include "profiling.h"

#include <cmath>

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
inline Vector3  operator - (Vector3 v);

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
inline Matrix4 rotate(Matrix4 m, f32 theta, Vector3 axis);
inline Matrix4 rotate_x(Matrix4 m, f32 theta);
inline Matrix4 rotate_y(Matrix4 m, f32 theta);
inline Matrix4 rotate_z(Matrix4 m, f32 theta);
inline Matrix4 look_at(Vector3 eye, Vector3 origin, Vector3 up);

/*******************************************************************************
 * Matrix4 operator declarations
 ******************************************************************************/
inline Vector3 operator * (Matrix4 lhs, Vector3 rhs);
inline Matrix4 operator * (Matrix4 lhs, Matrix4 rhs);


/*******************************************************************************
 * Vector3 function definitions
 ******************************************************************************/
inline f32 length(Vector3 vec)
{
    return lry::sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
}

inline f32 length_sq(Vector3 vec)
{
    return vec.x * vec.x + vec.y * vec.y + vec.z * vec.z;
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

f32 lerp(f32 a, f32 b, f32 t)
{
    return (1.0f-t) * a + t*b;
}

Vector3 lerp(Vector3 a, Vector3 b, f32 t)
{
    Vector3 r;

    r.x = lerp(a.x, b.x, t);
    r.y = lerp(a.y, b.y, t);
    r.z = lerp(a.z, b.z, t);

    return r;
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

Vector3 surface_normal(Vector3 v0, Vector3 v1, Vector3 v2)
{
    Vector3 n;

    Vector3 u = v1 - v0;
    Vector3 v = v2 - v0;

    n = cross(u, v);
    n = normalise(n);

    return n;
}

inline Matrix4 translate(Matrix4 m, Vector3 v)
{
    Matrix4 result = m;
    result[3].x += v.x;
    result[3].y += v.y;
    result[3].z += v.z;
    return result;
}

inline Matrix4 translate(Matrix4 m, f32 s)
{
    Matrix4 result = m;
    result[3].x += s;
    result[3].y += s;
    result[3].z += s;
    return result;
}

inline Matrix4 scale(Matrix4 m, Vector3 s)
{
    Matrix4 result = m;
    result[0].x = result[0].x * s.x;
    result[1].y = result[1].y * s.y;
    result[2].z = result[2].z * s.z;
    return result;
}

inline Matrix4 scale(Matrix4 m, f32 s)
{
    Matrix4 result = m;
    result[0].x = result[0].x * s;
    result[1].y = result[1].y * s;
    result[2].z = result[2].z * s;
    return result;
}

inline Matrix4 translate(Matrix4 m, Vector2 v)
{
    Matrix4 result = m;
    result[3].x += v.x;
    result[3].y += v.y;
    return result;
}

inline Matrix4 rotate(Matrix4 m, f32 theta, Vector3 axis)
{
    f32 c = lry::cos(theta);
    f32 s = lry::cos(theta);

    Vector3 tmp = (1.0f - c) * axis;

    Matrix4 r;
    r[0].x = c + tmp.x * axis.x;
    r[0].y = tmp.x * axis.y + s * axis.z;
    r[0].z = tmp.x * axis.z - s * axis.y;

    r[1].x = tmp.y * axis.x - s * axis.z;
    r[1].y = c + tmp.y * axis.y;
    r[1].z = tmp.y * axis.z + s * axis.x;

    r[2].x = tmp.z * axis.x + s * axis.y;
    r[2].y = tmp.z * axis.y - s * axis.x;
    r[2].z = c + tmp.z * axis.z;

    Matrix4 result;
    result[0] = m[0] * r[0].x + m[1] * r[0].y + m[2] * r[0].z;
    result[1] = m[0] * r[1].x + m[1] * r[1].y + m[2] * r[1].z;
    result[2] = m[0] * r[2].x + m[1] * r[2].y + m[2] * r[2].z;
    result[3] = m[3];
    return result;
}

inline Matrix4 rotate_x(Matrix4 m, f32 theta)
{
    f32 c = lry::cos(theta);
    f32 s = lry::cos(theta);

    Matrix4 r;
    r[0].x = 1;
    r[0].y = 0;
    r[0].z = 0;

    r[1].x = 0;
    r[1].y = c;
    r[1].z = s;

    r[2].x = 0;
    r[2].y = -s;
    r[2].z = c;

    Matrix4 result;
    result[0] = m[0] * r[0].x;
    result[1] = m[1] * r[1].y + m[2] * r[1].z;
    result[2] = m[1] * r[2].y + m[2] * r[2].z;
    result[3] = m[3];
    return result;
}

inline Matrix4 rotate_y(Matrix4 m, f32 theta)
{
    f32 c = lry::cos(theta);
    f32 s = lry::cos(theta);

    Matrix4 r;
    r[0].x = c;
    r[0].y = 0;
    r[0].z = s;

    r[1].x = 0;
    r[1].y = 1;
    r[1].z = 0;

    r[2].x = -s;
    r[2].y = 0;
    r[2].z = c;

    Matrix4 result;
    result[0] = m[0] * r[0].x + m[2] * r[0].z;
    result[1] = m[1] * r[1].y;
    result[2] = m[0] * r[2].x + m[2] * r[2].z;
    result[3] = m[3];
    return result;
}

inline Matrix4 rotate_z(Matrix4 m, f32 theta)
{
    f32 c = lry::cos(theta);
    f32 s = lry::cos(theta);

    Matrix4 r;
    r[0].x = c;
    r[0].y = s;
    r[0].z = 0;

    r[1].x = -s;
    r[1].y = c;
    r[1].z = 0;

    r[2].x = 0;
    r[2].y = 0;
    r[2].z = 1;

    Matrix4 result;
    result[0] = m[0] * r[0].x + m[1] * r[0].y;
    result[1] = m[0] * r[1].x + m[1] * r[1].y;
    result[2] = m[2] * r[2].z;
    result[3] = m[3];

    return result;
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

inline Vector3 operator/(Vector3 lhs, f32 rhs)
{
    Vector3 v;
    v.x = lhs.x / rhs;
    v.y = lhs.y / rhs;
    v.z = lhs.z / rhs;
    return v;
}

inline Vector2 operator * (Matrix4 lhs, Vector2 rhs)
{
    Vector2 result = {};
    result.x = lhs[0].x * rhs.x + lhs[0].y * rhs.y + lhs[3].x;
    result.y = lhs[1].x * rhs.x + lhs[1].y * rhs.y + lhs[3].y;
    return result;
}

inline Vector2& operator *=(Vector2 &lhs, f32 rhs)
{
    lhs.x *= rhs;
    lhs.y *= rhs;
    return lhs;
}

inline Vector2 operator+ (Vector2 lhs, Vector2 rhs)
{
    Vector2 result = {};
    result.x = lhs.x + rhs.x;
    result.y = lhs.y + rhs.y;
    return result;
}

inline Vector2& operator+=(Vector2 &lhs, f32 rhs)
{
    lhs.x += rhs;
    lhs.y += rhs;
    return lhs;
}

inline Matrix4 operator * (Matrix4 lhs, Matrix4 rhs)
{
    Matrix4 result = {};
    result[0].x = lhs[0].x * rhs[0].x + lhs[1].x * rhs[0].y + lhs[2].x * rhs[0].z + lhs[3].x * rhs[0].w;
    result[0].y = lhs[0].y * rhs[0].x + lhs[1].y * rhs[0].y + lhs[2].y * rhs[0].z + lhs[3].y * rhs[0].w;
    result[0].z = lhs[0].z * rhs[0].x + lhs[1].z * rhs[0].y + lhs[2].z * rhs[0].z + lhs[3].z * rhs[0].w;
    result[0].w = lhs[0].w * rhs[0].x + lhs[1].w * rhs[0].y + lhs[2].w * rhs[0].z + lhs[3].w * rhs[0].w;

    result[1].x = lhs[0].x * rhs[1].x + lhs[1].x * rhs[1].y + lhs[2].x * rhs[1].z + lhs[3].x * rhs[1].w;
    result[1].y = lhs[0].y * rhs[1].x + lhs[1].y * rhs[1].y + lhs[2].y * rhs[1].z + lhs[3].y * rhs[1].w;
    result[1].z = lhs[0].z * rhs[1].x + lhs[1].z * rhs[1].y + lhs[2].z * rhs[1].z + lhs[3].z * rhs[1].w;
    result[1].w = lhs[0].w * rhs[1].x + lhs[1].w * rhs[1].y + lhs[2].w * rhs[1].z + lhs[3].w * rhs[1].w;

    result[2].x = lhs[0].x * rhs[2].x + lhs[1].x * rhs[2].y + lhs[2].x * rhs[2].z + lhs[3].x * rhs[2].w;
    result[2].y = lhs[0].y * rhs[2].x + lhs[1].y * rhs[2].y + lhs[2].y * rhs[2].z + lhs[3].y * rhs[2].w;
    result[2].z = lhs[0].z * rhs[2].x + lhs[1].z * rhs[2].y + lhs[2].z * rhs[2].z + lhs[3].z * rhs[2].w;
    result[2].w = lhs[0].w * rhs[2].x + lhs[1].w * rhs[2].y + lhs[2].w * rhs[2].z + lhs[3].w * rhs[2].w;

    result[3].x = lhs[0].x * rhs[3].x + lhs[1].x * rhs[3].y + lhs[2].x * rhs[3].z + lhs[3].x * rhs[3].w;
    result[3].y = lhs[0].y * rhs[3].x + lhs[1].y * rhs[3].y + lhs[2].y * rhs[3].z + lhs[3].y * rhs[3].w;
    result[3].z = lhs[0].z * rhs[3].x + lhs[1].z * rhs[3].y + lhs[2].z * rhs[3].z + lhs[3].z * rhs[3].w;
    result[3].w = lhs[0].w * rhs[3].x + lhs[1].w * rhs[3].y + lhs[2].w * rhs[3].z + lhs[3].w * rhs[3].w;
    return result;
}


/*******************************************************************************
 * Vector3 operator definitions
 ******************************************************************************/
inline Vector3  operator - (Vector3 v)
{
    return { -v.x, -v.y, -v.z };
}

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

/*******************************************************************************
 * Quaternion operations
 ******************************************************************************/
inline f32 length(Quaternion q)
{
    return lry::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
}

inline Quaternion normalise(Quaternion q)
{
    Quaternion r;

    f32 il = 1.0f / length(q);
    r = { q.x * il, q.y * il, q.z * il, q.w * il };
    return r;
}

inline Quaternion inverse(Quaternion q)
{
    Quaternion r;
    r = { -q.x, -q.y, -q.z, q.w };
    return r;
}

inline Quaternion operator* (Quaternion p, Quaternion q)
{
    Quaternion r;
    r.x = p.w * q.x + p.x * q.w + p.y * q.z - p.z * q.y;
    r.y = p.w * q.y + p.y * q.w + p.z * q.x - p.x * q.z;
    r.z = p.w * q.z + p.z * q.w + p.x * q.y - p.y * q.x;
    r.w = p.w * q.w - p.x * q.x - p.y * q.y - p.z * q.z;
    return r;
}

inline Vector3 rotate(Vector3 v, Quaternion p)
{
    Vector3 r;

    Quaternion vq = Quaternion::make(v);
    Quaternion rq = p * vq * inverse(p);

    r = { rq.x, rq.y, rq.z };
    return r;
}

i32 factorial(i32 x)
{
    i32 result = 1;
    for (i32 i = 2; i <= x; i++) {
        result *= i;
    }
    return result;
}

#if LEARY_ENABLE_SSE2
#include <emmintrin.h>

#if defined(_MSC_VER)
#define ALIGN16_BEG _declspec(align(16))
#define ALGIN16_END
#else
#define ALIGN16_BEG
#define ALIGN16_END __atribute__((aligned(16)))
#endif

#define PS_CONST(name, val) \
    f32 name[4] = { (f32)val, (f32)val, (f32)val, (f32)val }
#define PI32_CONST(name, val) \
    i32 name[4] = { val, val, val, val }
#define PS_CONST_TYPE(name, type, val) \
    type name[4] = { val, val, val, val }

PS_CONST(ps_1,   1.0f);
PS_CONST(ps_0p5, 0.5f);

PS_CONST_TYPE(ps_sign_mask,     i32, (i32) 0x80000000);
PS_CONST_TYPE(ps_inv_sign_mask, i32, (i32)~0x80000000);

PI32_CONST(pi32_1,     1);
PI32_CONST(pi32_inv1, ~1);
PI32_CONST(pi32_2,     2);
PI32_CONST(pi32_4,     4);
PI32_CONST(pi32_0x7f,  0x7f);

PS_CONST(ps_minus_cephes_DP1, -0.78515625);
PS_CONST(ps_minus_cephes_DP2, -2.4187564849853515625e-4);
PS_CONST(ps_minus_cephes_DP3, -3.77489497744594108e-8);
PS_CONST(ps_sincof_p0,        -1.9515295891E-4);
PS_CONST(ps_sincof_p1,        8.3321608736E-3);
PS_CONST(ps_sincof_p2,        -1.6666654611E-1);
PS_CONST(ps_coscof_p0,        2.443315711809948E-005);
PS_CONST(ps_coscof_p1,        -1.388731625493765E-003);
PS_CONST(ps_coscof_p2,        4.166664568298827E-002);
PS_CONST(ps_cephes_FOPI,      1.27323954473516);

namespace lry {
    f32 cos_taylor(f32 x)
    {
        // TODO(jesper): faster aproximation with sse2
        f32 result;

        f32 x2 = x*x;
        f32 x4 = x2*x2;
        f32 x6 = x4*x2;
        f32 x8 = x6*x2;
        f32 x10 = x8*x2;
        f32 x12 = x10*x2;
        f32 x14 = x12*x2;

        i32 fac2 = factorial(2);
        i32 fac4 = factorial(4);
        i32 fac6 = factorial(6);
        i32 fac8 = factorial(8);
        i32 fac10 = factorial(10);
        i32 fac12 = factorial(12);
        i32 fac14 = factorial(14);

        result = 1 - x2/fac2 + x4/fac4 - x6/fac6 + x8/fac8 - x10/fac10 + x12/fac12 - x14/fac14;
        return result;
    }
    f32 cos(f32 x)
    {
        return cos_taylor(x);
    }

    f32 sin_taylor(f32 x)
    {
        // TODO(jesper): faster aproximation with sse2
        f32 result;

        f32 x2 = x*x;
        f32 x3 = x2*x;
        f32 x5 = x3*x2;
        f32 x7 = x5*x2;
        f32 x9 = x7*x2;
        f32 x11 = x9*x2;
        f32 x13 = x11*x2;
        f32 x15 = x13*x2;

        i32 fac3 = factorial(3);
        i32 fac5 = factorial(5);
        i32 fac7 = factorial(7);
        i32 fac9 = factorial(9);
        i32 fac11 = factorial(11);
        i32 fac13 = factorial(13);
        i32 fac15 = factorial(15);

        result = x - x3/fac3 + x5/fac5 - x7/fac7 + x9/fac9 - x11/fac11 + x13/fac13 - x15/fac15;
        return result;
    }

    f32 sin_cephes(f32 x)
    {
        __m128  xmm0, xmm1, xmm2, xmm3, sign_bit, y;
        __m128i emm0, emm1;

        xmm0 = _mm_set1_ps(x);
        xmm2 = _mm_setzero_ps();

        sign_bit = xmm0;
        xmm0 = _mm_and_ps(xmm0, *(__m128*)ps_inv_sign_mask);
        sign_bit = _mm_and_ps(sign_bit, *(__m128*)ps_sign_mask);

        y = _mm_mul_ps(xmm0, *(__m128*)ps_cephes_FOPI);
        emm1 = _mm_cvttps_epi32(y);
        emm1 = _mm_add_epi32(emm1, *(__m128i*)pi32_1);
        emm1 = _mm_and_si128(emm1, *(__m128i*)pi32_inv1);
        y = _mm_cvtepi32_ps(emm1);

        emm0 = _mm_and_si128(emm1, *(__m128i*)pi32_4);
        emm0 = _mm_slli_epi32(emm0, 29);

        emm1 = _mm_and_si128(emm1, *(__m128i*)pi32_2);
        emm1 = _mm_cmpeq_epi32(emm1, _mm_setzero_si128());

        __m128 swap_sign_bit = _mm_castsi128_ps(emm0);
        __m128 poly_mask     = _mm_castsi128_ps(emm1);
        sign_bit = _mm_xor_ps(sign_bit, swap_sign_bit);

        xmm1 = *(__m128*)ps_minus_cephes_DP1;
        xmm2 = *(__m128*)ps_minus_cephes_DP2;
        xmm3 = *(__m128*)ps_minus_cephes_DP3;

        xmm1 = _mm_mul_ps(y, xmm1);
        xmm2 = _mm_mul_ps(y, xmm2);
        xmm3 = _mm_mul_ps(y, xmm3);

        xmm0 = _mm_add_ps(xmm0, xmm1);
        xmm0 = _mm_add_ps(xmm0, xmm2);
        xmm0 = _mm_add_ps(xmm0, xmm3);

        y = *(__m128*)ps_coscof_p0;
        __m128 z = _mm_mul_ps(xmm0, xmm0);

        y = _mm_mul_ps(y, z);
        y = _mm_add_ps(y, *(__m128*)ps_coscof_p1);
        y = _mm_mul_ps(y, z);
        y = _mm_add_ps(y, *(__m128*)ps_coscof_p2);
        y = _mm_mul_ps(y, z);
        y = _mm_mul_ps(y, z);
        __m128 tmp = _mm_mul_ps(z, *(__m128*)ps_0p5);
        y = _mm_sub_ps(y, tmp);
        y = _mm_add_ps(y, *(__m128*)ps_1);

        __m128 y2 = *(__m128*)ps_sincof_p0;
        y2 = _mm_mul_ps(y2, z);
        y2 = _mm_add_ps(y2, *(__m128*)ps_sincof_p1);
        y2 = _mm_mul_ps(y2, z);
        y2 = _mm_add_ps(y2, *(__m128*)ps_sincof_p2);
        y2 = _mm_mul_ps(y2, z);
        y2 = _mm_mul_ps(y2, xmm0);
        y2 = _mm_mul_ps(y2, xmm0);

        xmm3 = poly_mask;
        y2 = _mm_and_ps(xmm3, y2);
        y  = _mm_andnot_ps(xmm3, y);
        y  = _mm_add_ps(y, y2);
        y  = _mm_xor_ps(y, sign_bit);

        return _mm_cvtss_f32(y);
    }

    f32 sin(f32 x)
    {
        return sin_taylor(x);
    }

    f32 tan(f32 x)
    {
        // TODO(jesper): division by zero
        // TODO(jesper): faster aproximation with sse2
        return lry::cos(x) / lry::cos(x);
    }

    f32 ceil(f32 f)
    {
        return (f32)(i32)(f + 0.5f);
    }

    f32 floor(f32 f)
    {
        return (f32)(i32)f;
    }

    f32 sqrt(f32 f)
    {
        __m128 val = _mm_set1_ps(f);
        __m128 res = _mm_sqrt_ps(val);
        f32 resf   = _mm_cvtss_f32(res);
        return resf;
    }

    f32 abs(f32 f)
    {
        return lry::sqrt(f*f);
    }
} // namespace lry

#endif

extern Settings g_settings;
Vector2 camera_from_screen( Vector2 v )
{
    Vector2 r;
    r.x = 2.0f * v.x / g_settings.video.resolution.width - 1.0f;
    r.y = 2.0f * v.y / g_settings.video.resolution.height - 1.0f;
    return r;
}

Vector3 camera_from_screen( Vector3 v )
{
    Vector3 r;
    r.x = 2.0f * v.x / g_settings.video.resolution.width - 1.0f;
    r.y = 2.0f * v.y / g_settings.video.resolution.height - 1.0f;
    r.z = v.z;
    return r;
}


Vector2 screen_from_camera( Vector2 v)
{
    Vector2 r;
    r.x = g_settings.video.resolution.width  / 2.0f * ( v.x + 1.0f );
    r.y = g_settings.video.resolution.height / 2.0f * ( v.y + 1.0f );
    return r;
}

Vector3 screen_from_camera( Vector3 v)
{
    Vector3 r;
    r.x = g_settings.video.resolution.width  / 2.0f * ( v.x + 1.0f );
    r.y = g_settings.video.resolution.height / 2.0f * ( v.y + 1.0f );
    r.z = v.z;
    return r;
}


