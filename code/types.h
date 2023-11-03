#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <float.h>
#include <stdbool.h>
#include <assert.h>

#if defined(_WIN32) || defined(WIN32)
# define PLATFORM_WIN32 1
#else 
# define PLATFORM_WIN32 0
#endif

#if defined(_DEBUG) || (DEBUG)
# define DEBUG_BUILD 1
#else 
# define DEBUG_BUILD 0
#endif

typedef char8_t   c8;
typedef char16_t  c16;
typedef char32_t  c32;

typedef int8_t    s8;
typedef int16_t   s16;
typedef int32_t   s32;
typedef int64_t   s64;

typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;

typedef uint8_t    b8;
typedef uint16_t   b16;
typedef uint32_t   b32;
typedef uint64_t   b64;

typedef float     f32;
typedef double    f64;

typedef uintptr_t uptr;
typedef intptr_t  sptr;

union u128
{
    struct { s64 Upper, Lower; };
    s64 Bits[2];
};

#define U8_MAX   UINT8_MAX
#define U16_MAX  UINT16_MAX
#define U32_MAX  UINT32_MAX
#define U64_MAX  UINT64_MAX
#define I8_MAX   INT8_MAX
#define I16_MAX  INT16_MAX
#define I32_MAX  INT32_MAX
#define I64_MAX  INT64_MAX
#define F32_MIN -FLT_MAX
#define F32_MAX  FLT_MAX
#define F64_MIN -DBL_MAX
#define F64_MAX  DBL_MAX

#define fn            extern "C"
#define fn_internal   static
#define fn_inline     inline
#define fn_imported   fn __declspec( dllimport )
#define fn_exported   fn __declspec( dllexport )

#define var_persist   static
#define var_global    static

#define _KB(x) (x * 1024)
#define _MB(x) (_KB(x) * 1024)
#define _GB(x) (_MB(x) * 1024)

#define _64KB  _KB(64)
#define _1MB   _MB(1)
#define _2MB   _MB(2)
#define _4MB   _MB(4)
#define _8MB   _MB(8)
#define _16MB  _MB(16)
#define _32MB  _MB(32)
#define _64MB  _MB(64)
#define _128MB _MB(128)
#define _256MB _MB(256)
#define _1GB   _GB(1)

// A hack to determine if platform is little endian
// TODO(enlynn): Do something that isn't so hacky.
#define LITTLE_ENDIAN 0x41424344UL 
#define ENDIAN_ORDER  ('ABCD') 
#define IS_LITTLE_ENDIAN (ENDIAN_ORDER==LITTLE_ENDIAN)

#define ForwardAlign(Base, Alignment) (((u64)(Base) + (u64)(Alignment) - 1) & ~((u64)(Alignment) - 1))
// I doubt this is the most efficient way of doing things, but it is easy to understand and does not run the risk of an underflow
#define BackwardAlign(Base, Alignment) (ForwardAlign(((Base) + 1), Alignment) - (Alignment))
#define DivideAlign(val, align) (((val) + (align) - 1) / (align))
// Should handle overflow and +/- numbers
// TODO(enlynn): Maybe write out as a define?
template<typename T> 
constexpr T DivideCeil(T Numerator, T Denominator)
{
    return (Numerator > 0) ? (1 + ((Numerator - 1) / Denominator)) : (Numerator / Denominator);
}

#define ArrayCount(array)    (sizeof(array) / sizeof(array[0]))
#define Clamp(Val, Min, Max) (((Val) < (Min)) ? (Min) : (((Val) > (Max)) ? (Max) : (Val)))

#define ForRange(Type, VarName, Count)        for (Type VarName = 0;           VarName < (Count); ++VarName)
#define ForRangeReverse(Type, VarName, Count) for (Type VarName = (Count) - 1; VarName >= 0;      --VarName)

// source: https://hbfs.wordpress.com/2008/08/05/branchless-equivalents-of-simple-functions/
// short for sign extend :p
// forces the compile to use cbw family of instructions (ideally)
fn_inline int
Sex(int x)
{
    union
    {
        s64 w;
        struct { s32 lo, hi; } _p;
    } z;
    z.w = x;
    return z._p.hi;
}

fn_inline s32
FastSign32(s32 v)
{
    return (((u64)-v >> 31) - ((u64)v >> 31));
}

fn_inline s32
FastSign64(s64 v)
{
    return (((u64)-v >> 63) - ((u64)v >> 63));
}

fn_inline int
FastAbs(int x)
{
    int result;
    result = (x ^ Sex(x)) - Sex(x);
    return result;
}

fn_inline int
FastMax(int min, int max)
{
    int result;
    result = min + ((max - min) & ~Sex(max - min));
    return result;
}

fn_inline int
FastMin(int min, int max)
{
    int result;
    result = max + ((min - max) & Sex(min - max));
    return result;
}

/**
    * Round up to the next highest power of 2.
    * @source: http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
    */
fn_inline u32
NextHighestPow2_u32(u32 v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

/**
* Round up to the next highest power of 2.
* @source: http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
*/
fn_inline u64
NextHighestPow2_u64(u64 v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v |= v >> 32;
    v++;
    return v;
}