#include "str16.h"
#include <string.h>

//#define STRING_NO_UNPAIRED_SURROGATES
#define STRING_REPLACEMENT_CHAR 0xfffd

u64 
Strlen16(const c16* Ptr)
{
    int Index = 0;
    u64 Size = 0;
    while (Ptr[Index])
    {
        int consumed;
        char32_t CodePoint = ToCodePoint(&Ptr[Index], &consumed);
        Index += consumed;
        Size += SizeInUTF8(CodePoint);
    }
    return Size;
}

bool ToUTF8(char32_t cp, char out[4], int* size);
bool ToUTF16(char32_t cp, char16_t out[2], int* size);

// https://en.wikipedia.org/wiki/UTF-8#Encoding
bool ToUTF8(char32_t cp, char out[4], int* size)
{
    *size = SizeInUTF8(cp);
    switch (*size)
    {
    case 1:
    {
        out[0] = (char)(cp);
        return true;
    }
    case 2:
    {
        out[0] = 0xc0 | ((cp >> 6) & 0x1f);
        out[1] = 0x80 | (cp & 0x3f);
        return true;
    }
    case 3:
    {
        out[0] = 0xe0 | ((cp >> 12) & 0x0f);
        out[1] = 0x80 | ((cp >> 6) & 0x3f);
        out[2] = 0x80 | (cp & 0x3f);
        return true;
    }
    case 4:
    {
        out[0] = 0xf0 | ((cp >> 18) & 0x07);
        out[1] = 0x80 | ((cp >> 12) & 0x3f);
        out[2] = 0x80 | ((cp >> 6) & 0x3f);
        out[3] = 0x80 | (cp & 0x3f);
        return true;
    }
    default:
    {
        *size = 3;
        out[0] = (unsigned char)(0xef);
        out[1] = (unsigned char)(0xbf);
        out[2] = (unsigned char)(0xbd);
        return false;
    }
    }
}

// I'm not going to explain how these are decoded, you'll have to read this:
// https://en.wikipedia.org/wiki/UTF-16#Description
bool ToUTF16(char32_t cp, char16_t out[2], int* size)
{
    *size = SizeInUTF16(cp);
    switch (*size)
    {
    case 1:
    {
        out[0] = (char16_t)(cp);
        return true;
    }
    case 2:
    {
        cp -= 0x10000;
        out[0] = (char16_t)((cp >> 10) + 0xd800);
        out[1] = (cp & 0x3ff) + 0xdc00;
        return true;
    }
    default:
    {
        *size = 1;
        out[0] = 0xfffd;
        return false;
    }
    }
}

int SizeInUTF8(c32 cp)
{
    if (cp <= 0x7f)                   return 1;
    if (cp <= 0x7ff)                  return 2;
#ifdef STRING_NO_UNPAIRED_SURROGATES
    if (cp >= 0xd800 && cp <= 0xdfff) return 0; // Invalid codepoint reserved for UTF-16 surrogate pairs.
#endif
    if (cp <= 0xfffd)                 return 3;
    if (cp <= 0xffff)                 return 0; // 0xfffe and 0xffff are invalid.
    if (cp <= 0x10ffff)               return 4;

    return 0; // Anything above 0x10ffff is invalid.
}

int SizeInUTF16(c32 cp)
{
    if (cp <= 0xd7ff)   return 1; // Low range of single-word codes.
#ifdef STRING_NO_UNPAIRED_SURROGATES
    if (cp <= 0xdfff)   return 0; // Invalid codepoint reserved for UTF-16 surrogate pairs.
#endif
    if (cp <= 0xfffd)   return 1; // High range of single-word codes.
    if (cp <= 0xffff)   return 0; // 0xfffe and 0xffff are invalid.
    if (cp <= 0x10ffff) return 2; // Anything above 0xffff needs two words.

    return 0; // Anything above 0x10ffff is invalid.
}

int SizeFromLeading(const char c)
{
    if ((c & 0x80) == 0x00)                 return 1;
    if (c == 0xc0 || c == 0xc1 || c > 0xf4) return 0; // Illegal UTF-8.
    if ((c & 0xe0) == 0xc0)                 return 2;
    if ((c & 0xf0) == 0xe0)                 return 3;
    if ((c & 0xf8) == 0xf0)                 return 4;

    return 0; // Unrecognized leading byte.
}

int SizeFromLeading(const c16 c)
{
    if (c <= 0xd7ff) return 1; // Low range of single-word codes.
    if (c <= 0xdbff) return 2; // High surrogate of double-word codes.
#ifdef STRING_NO_UNPAIRED_SURROGATES
    if (c <= 0xdfff) return 0; // Illegal, trailing surrogate cannot be the leading word.
#endif
    if (c <= 0xfffd) return 1; // High range of single-word surrogates.

    return 0; // Codepoints 0xfffe and 0xffff are invalid.
}

// https://en.wikipedia.org/wiki/UTF-8#Encoding
c32 ToCodePoint(const char utf8[4], int* consumed)
{
    *consumed = SizeFromLeading(utf8[0]);
    switch (*consumed)
    {
    case 1: return utf8[0] & 0x7f;
    case 2:
    {
        if ((utf8[1] & 0xc0) != 0x80) break;
        return ((utf8[0] & 0x1f) << 6) | (utf8[1] & 0x3f);
    }
    case 3:
    {
        if ((utf8[1] & 0xc0) != 0x80 || (utf8[2] & 0xc0) != 0x80) break;
        char32_t out = ((utf8[0] & 0x0f) << 12) | ((utf8[1] & 0x3f) << 6) | (utf8[2] & 0x3f);
        if (utf8[0] == 0xe0 && out < 0x0800) break;
        if (utf8[0] == 0xe4 && out > 0xd800 && out < 0xdfff) break;
        return out;
    }
    case 4:
    {
        if ((utf8[1] & 0xc0) != 0x80 || (utf8[2] & 0xc0) != 0x80 || (utf8[3] & 0xc0) != 0x80) break;
        char32_t out = ((utf8[0] & 0x07) << 18) | ((utf8[1] & 0x3f) << 12) | ((utf8[2] & 0x3f) << 6) | (utf8[3] & 0x3f);
        if (utf8[0] == 0xf0 && out < 0x10000) break;
        if (utf8[0] == 0xf4 && out > 0x10ffff) break;
        return out;
    }
    }

    *consumed = 1; // TODO(Frog): If we can't decode, should we assume the size was correct, or just skip one byte?
    return STRING_REPLACEMENT_CHAR;
}

// https://en.wikipedia.org/wiki/UTF-16#Description
c32 ToCodePoint(const c16 utf16[2], int* consumed)
{
    *consumed = SizeFromLeading(utf16[0]); // Get the number of words in the codepoint.
    switch (*consumed)
    {
    case 1: return utf16[0]; // Single word, just return it as-is.
    case 2: // Surrogate pair. Combine and return, or if the option is enabled, validate the pair.
    {
#ifdef STRING_NO_UNPAIRED_SURROGATES
        if (utf16[1] < 0xdc00 || utf16[1] > 0xdfff) break;
#endif
        return ((utf16[0] - 0xd800) << 10) + (utf16[1] - 0xdc00) + 0x10000;
    }
    }
    *consumed = 1; // TODO(Frog): Unlike UTF-8, I think it makes sense to skip one word at a time here. Revisit later.
    return STRING_REPLACEMENT_CHAR;
}

inline bool operator==(istr16 Lhs, istr16 Rhs)
{
    return (Lhs.Length() == Rhs.Length() && memcmp((void*)Lhs.Ptr(), Rhs.Ptr(), Lhs.Length()) == 0);
}

inline bool operator==(istr16 Lhs, const c16* Rhs)
{
    return (Lhs.Length() == Strlen16(Rhs) && memcmp((void*)Lhs.Ptr(), Rhs, Lhs.Length()) == 0);
}

inline bool operator==(const c16* Lhs, istr16 Rhs)
{
    return (Strlen16(Lhs) == Rhs.Length() && memcmp((void*)Lhs, Rhs.Ptr(), Rhs.Length()) == 0);
}

istr16::istr16(const c16* Ptr)
{
    mBorrowedPtr = Ptr;
    mLen         = Strlen16(Ptr);
}
