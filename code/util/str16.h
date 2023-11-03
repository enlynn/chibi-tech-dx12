#pragma once

#include <types.h>

u64 Strlen16(const c16* NullTerminatedStr);

bool ToUTF8(char32_t cp, char out[4], int* size);
bool ToUTF16(char32_t cp, char16_t out[2], int* size);
int SizeInUTF8(c32 cp);
int SizeInUTF16(c32 cp);
int SizeFromLeading(const char c);
int SizeFromLeading(const c16 c);
c32 ToCodePoint(const char utf8[4], int* consumed);
c32 ToCodePoint(const c16 utf16[2], int* consumed);

/// <summary>
/// Immutable String type than can hodl UTF-16 strings. This is a 
/// conviencience type for passing around (const char16_t*, length) to functions.
/// 
/// The string is *not* owned by this type.
/// </summary>
struct istr16
{
public:
    istr16() = delete;
    istr16(const c16* Ptr);
    constexpr istr16(const c16* Ptr, u64 Length) : mBorrowedPtr(Ptr), mLen(Length) {}

    constexpr operator const c16* () const { return mBorrowedPtr; }

    constexpr u64        Length()    const { return mLen; }
    constexpr const c16* Ptr()       const { return mBorrowedPtr; }

    constexpr const c16& operator[](u64 Index) const { return Ptr()[Index]; }

    // Legacy iterators
    constexpr const c16* begin() const { return Ptr(); }
    constexpr const c16* end()   const { return Ptr() + Length(); }

    // Comparison operators. Comparison with mstr16 is implemented inside of mstr16.
    inline friend bool operator==(istr16 lhs, istr16 rhs);
    inline friend bool operator==(istr16 lhs, const c16* rhs);
    inline friend bool operator==(const c16* lhs, istr16 rhs);

    inline friend bool operator!=(istr16 lhs, istr16 rhs)     { return !(lhs == rhs); }
    inline friend bool operator!=(const c16* lhs, istr16 rhs) { return !(lhs == rhs); }
    inline friend bool operator!=(istr16 lhs, const c16* rhs) { return !(lhs == rhs); }

private:
    const c16*  mBorrowedPtr;
    u64         mLen;
};