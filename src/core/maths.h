/**
 * file:    math.h
 * created: 2017-03-20
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

namespace lry {
    f32 cos(f32 x);
    f32 sin(f32 x);
    f32 tan(f32 x);
    f32 sqrt(f32 x);
    f32 ceil(f32 x);
    f32 floor(f32 x);
    f32 abs(f32 x);
}

#define PI (f32)M_PI

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

union Vector2i {
    i32 data[2];
};

union Vector3i {
    i32 data[3];
    struct {
        i32 x, y, z;
    };
    struct {
        i32 r, g, b;
    };

    struct {
        Vector2i xy;
        i32 DUMMY_VAR;
    };
    struct {
        Vector2i rg;
        i32 DUMMY_VAR;
    };

    struct {
        i32 DUMMY_VAR;
        Vector2i yz;
    };
    struct {
        i32 DUMMY_VAR;
        Vector2i gb;
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

    f32& operator[](i32 i)
    {
        ASSERT(i < 4);
        return data[i];
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

    static inline Quaternion make(Vector4 v)
    {
        return { v.x, v.y, v.z, v.w };
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

struct Matrix4 {
    Vector4 columns[4];

    inline Vector4& operator[] (i32 i)
    {
        return columns[i];
    }
};


f32
clamp(f32 val, f32 min, f32 max)
{
    val = val > max ? max : val;
    val = val < min ? min : val;
    return val;
}

i32 clamp(i32 val, i32 min, i32 max)
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

template<typename T>
T min(T a, T b)
{
    return a < b ? a : b;
}

template<typename T>
T max(T a, T b)
{
    return a > b ? a : b;
}

Vector4 unpack_rgba(u32 hex)
{
    Vector4 c;

    u8 r = (hex >> 24) & 0xFF;
    u8 g = (hex >> 16) & 0xFF;
    u8 b = (hex >> 8)  & 0xFF;
    u8 a = hex & 0xFF;

    c.r = r / 255.0f;
    c.g = g / 255.0f;
    c.b = b / 255.0f;
    c.a = a / 255.0f;

    return c;
}

Quaternion quat_from_euler(Vector3 v);
