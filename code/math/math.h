//
// Created by enlynn on 11/6/2023.
//

#pragma once

#include <types.h>

#include <cmath>
#include <limits>

constexpr f32 F32_EPSILON = std::numeric_limits<float>::epsilon() * 0.5f;

struct f32x2
{
    union
    {
        struct { f32 X, Y, Z, W; };
        f32 Ptr[4];
    };
};

struct f32x3
{
    union
    {
        struct { f32 X, Y, Z;         };
        struct { f32x2 XY; f32 Pad0;  };
        struct { f32 Pad1; f32x2 YZ;  };
        f32 Ptr[3];
    };
};

struct f32x4
{
    union
    {
        struct { f32 X, Y, Z, W;         };
        struct { f32x2 XY;   f32x2 ZW;   };
        struct { f32x3 XYZ;  f32   Pad0; };
        struct { f32   Pad1; f32x3 YZW;  };
        f32 Ptr[4];
    };

    inline f32    Length();
    inline f32    LengthSq();
    inline f32x4& Norm();
};

struct quaternion
{
    union
    {
        struct { f32 X, Y, Z, Theta; };
        struct { f32x3 XYZ; f32 Pad0; };
        f32x4 Ptr;
    };
};

struct f32x33
{
    f32 Ptr[9];
};

struct f32x44
{
    f32 Ptr[16];
};

using vec2 = f32x2;
using vec3 = f32x3;
using vec4 = f32x4;
using mat3 = f32x33;
using mat4 = f32x44;

static constexpr f32x2 f32x2_zero = { .Ptr = { 0, 0               } };
static constexpr f32x3 f32x3_zero = { .Ptr = { 0, 0, 0        } };
static constexpr f32x4 f32x4_zero = { .Ptr = { 0, 0, 0, 0 } };

static constexpr f32x2 f32x2_one = { .Ptr = { 1, 1               } };
static constexpr f32x3 f32x3_one = { .Ptr = { 1, 1, 1        } };
static constexpr f32x4 f32x4_one = { .Ptr = { 1, 1, 1, 1 } };

inline bool F32IsNan(f32 Value)
{
    return std::isnan(Value);
}

inline bool F32IsInf(f32 Value)
{
    return std::isinf(Value);
}

inline f32 F32FusedMultiplyAdd(f32 A, f32 B, f32 C)
{ // A * B + C
    return std::fma(A, B, C);
}

inline f32 Lerp(f32 Min, f32 Max, f32 T)
{
    return F32FusedMultiplyAdd(T, Max - Min, Min);
}

inline f32 f32x4::Length()   { return sqrt(X * X + Y * Y + Z * Z); }
inline f32 f32x4::LengthSq() { return X * X + Y * Y + Z * Z;          }

inline f32x4&
f32x4::Norm()
{
    f32 Len = Length();
    if (Len != 0.0f)
    {
        X /= Len;
        Y /= Len;
        Z /= Len;
        W /= Len;
    }
    return *this;
}

constexpr f32
Dot(f32x4 Left, f32x4 Right)
{
    return Left.X * Right.X + Left.Y * Right.Y + Left.Z * Right.Z + Left.W * Right.W;
}

