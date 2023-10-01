#pragma once

#include "types.h"

//
// @sources:
// - Frogbottom
// - Enlynn
//

/// <summary>
/// Immutable String type than can hodl ASCII or UTF-8 strings. This is a 
/// conviencience type for passing around (const char*, length) to functions.
/// 
/// The string is *not* owned by this type.
/// </summary>
struct istr8
{
public:
    istr8() = delete;
    const istr8(const char* Ptr);
    constexpr istr8(const char* Ptr, u64 Length) : mBorrowedPtr(Ptr), mLen(Length) {}

    constexpr operator const char* () const { return mBorrowedPtr; }

    constexpr u64         Length()    const { return mLen;         }
    constexpr const char* Ptr()       const { return mBorrowedPtr; }

    constexpr const char& operator[](u64 Index) const { return Ptr()[Index]; }

    // Legacy iterators
    constexpr const char* begin() const { return Ptr();            }
    constexpr const char* end()   const { return Ptr() + Length(); }

    // Comparison operators. Comparison with mstr8 is implemented inside of mstr8.
    inline friend bool operator==(istr8 lhs,       istr8 rhs);
    inline friend bool operator==(istr8 lhs,       const char* rhs);
    inline friend bool operator==(const char* lhs, istr8 rhs);

    inline friend bool operator!=(istr8 lhs,       istr8 rhs)       { return !(lhs == rhs); }
    inline friend bool operator!=(const char* lhs, istr8 rhs)       { return !(lhs == rhs); }
    inline friend bool operator!=(istr8 lhs,       const char* rhs) { return !(lhs == rhs); }


private:
	const char* mBorrowedPtr;
	u64         mLen;
};

/// <summary>
/// Mutable String type than can hold ASCII or UTF-8 strings. For strings less than
/// 24 bytes, they will be stored on the stack. For strings 24 bytes and above, strings
/// will be heap allocated.
/// </summary>
struct mstr8
{
public:
    // Constructors. Default constructor produces a valid empty string.
    mstr8();
    mstr8(const char* Ptr, u64 Length);
    mstr8(const char* Ptr);

    // Construction from istr8 has to be explicit since it might allocate.
    explicit mstr8(istr8 IStr) : mstr8(IStr.Ptr(), IStr.Length()) {}

    static mstr8 Format(const char* StrFormat, ...);

    // Getters and setters for length and capacity and whatnot.
    constexpr u64  Length() const;
    constexpr u64  Capacity() const;
    constexpr bool IsHeap() const;
    void           SetLength(u64 NewLen);
    void           ExpandIfNeeded(u64 RequiredCapacity);
    void           ShrinkToFit();

    // Accessors for the raw pointer, auto-cast, and array subscript operators.
    constexpr const char* Ptr() const { return IsHeap() ? mData.Heap.Ptr : mData.Stack.Ptr; }
    constexpr char*       Ptr()       { return IsHeap() ? mData.Heap.Ptr : mData.Stack.Ptr; }

    constexpr operator istr8       () const { return istr8(Ptr(), Length()); }
    constexpr operator const char* () const { return Ptr();                  }
    constexpr operator char*       ()       { return Ptr();                  }

    constexpr const char& operator[](u64 Index) const { return Ptr()[Index]; }
    constexpr char&       operator[](u64 Index)       { return Ptr()[Index]; }

    // Legacy iterators
    constexpr char*       begin()       { return Ptr();            }
    constexpr char*       end()         { return Ptr() + Length(); }
    constexpr const char* begin() const { return Ptr();            }
    constexpr const char* end()   const { return Ptr() + Length(); }

    // Comparison operators.
    inline friend bool operator==(const mstr8& Lhs, const mstr8& Rhs);
    inline friend bool operator==(const mstr8& Lhs, istr8 Rhs);
    inline friend bool operator==(const mstr8& Lhs, const char* Rhs);
    inline friend bool operator==(istr8 Lhs,        const mstr8& Rhs);
    inline friend bool operator==(const char* Lhs,  const mstr8& Rhs);

    inline friend bool operator!=(const mstr8& Lhs, const mstr8& Rhs) { return !(Lhs == Rhs); }
    inline friend bool operator!=(const mstr8& Lhs, istr8 Rhs)        { return !(Lhs == Rhs); }
    inline friend bool operator!=(const mstr8& Lhs, const char* Rhs)  { return !(Lhs == Rhs); }
    inline friend bool operator!=(istr8 Lhs,        const mstr8& Rhs) { return !(Lhs == Rhs); }
    inline friend bool operator!=(const char* Lhs,  const mstr8& Rhs) { return !(Lhs == Rhs); }

    // These are the methods that do actual work. Most remaining methods and operators
    // will just inline a call to Insert(), and many are only here to remove type ambiguity.
    mstr8& Insert(u64 Index, const char* Str, u64 StrLen);
    mstr8& Remove(u64 Index, u64 Count);

    mstr8& Insert(u64 Index, const char* Str); // Defined in implementation since it has to call strlen().
    inline mstr8& Insert(u64 Index, const mstr8& Str)  { return Insert(Index, Str.Ptr(), Str.Length());    }
    inline mstr8& Insert(u64 Index, istr8 Str)         { return Insert(Index, Str.Ptr(), Str.Length());    }
    inline mstr8& Insert(u64 Index, char c)            { return Insert(Index, &c, 1);                      }

    mstr8& Prepend(const char* Str); // Defined in implementation since it has to call strlen().
    inline mstr8& Prepend(const char* Str, u64 length) { return Insert(0, Str, length);                    }
    inline mstr8& Prepend(const mstr8& Str)            { return Insert(0, Str.Ptr(), Str.Length());        }
    inline mstr8& Prepend(istr8 Str)                   { return Insert(0, Str.Ptr(), Str.Length());        }
    inline mstr8& Prepend(char c)                      { return Insert(0, &c, 1);                          }

    mstr8& Append(const char* Str); // Defined in implementation since it has to call strlen().
    inline mstr8& Append(const char* Str, u64 length)  { return Insert(Length(), Str, length);             }
    inline mstr8& Append(const mstr8& Str)             { return Insert(Length(), Str.Ptr(), Str.Length()); }
    inline mstr8& Append(istr8 Str)                    { return Insert(Length(), Str.Ptr(), Str.Length()); }
    inline mstr8& Append(char c)                       { return Insert(Length(), &c, 1);                   }

    inline mstr8& operator+=(const mstr8& Rhs)         { return Insert(Length(), Rhs); }
    inline mstr8& operator+=(const char* Rhs)          { return Insert(Length(), Rhs); }
    inline mstr8& operator+=(istr8 Rhs)                { return Insert(Length(), Rhs); }
    inline mstr8& operator+=(char Rhs)                 { return Insert(Length(), Rhs); }

    // Passing one argument by value and then returning it helps the compiler figure out that it should
    // use the move constructor when we chain a bunch of + operators together.
    inline friend mstr8 operator+(mstr8 Lhs, const mstr8& Rhs) { Lhs.Insert(Lhs.Length(), Rhs); return Lhs; }
    inline friend mstr8 operator+(mstr8 Lhs, const char* Rhs)  { Lhs.Insert(Lhs.Length(), Rhs); return Lhs; }
    inline friend mstr8 operator+(mstr8 Lhs, istr8 Rhs)        { Lhs.Insert(Lhs.Length(), Rhs); return Lhs; }
    inline friend mstr8 operator+(mstr8 Lhs, char Rhs)         { Lhs.Insert(Lhs.Length(), Rhs); return Lhs; }

    inline friend mstr8 operator+(const char* Lhs, mstr8 Rhs)  { Rhs.Insert(0, Lhs); return Rhs; }
    inline friend mstr8 operator+(istr8 Lhs, mstr8 Rhs)        { Rhs.Insert(0, Lhs); return Rhs; }
    inline friend mstr8 operator+(char Lhs, mstr8 Rhs)         { Rhs.Insert(0, Lhs); return Rhs; }

    // Copy and move constructor/assignment nonsense.    
    mstr8(const mstr8& other);
    mstr8(mstr8&& other);
    mstr8& operator=(const mstr8& other);
    mstr8& operator=(mstr8&& other);

    // Destructor (or you can call Free() to deallocate).
    void Free();
    ~mstr8() { Free(); }

private:
    union
    {
        struct
        {
            char Ptr[16];
        } Stack;

        struct
        {
            char* Ptr;
            //
            // If stack allocation:
            // - Field is overwritten and unused
            //
            // If heap allocation:
            // - Field is treated as the length
            //
            u64   Length;
        } Heap;
    } mData;

    //
    // If stack allocation:
    // - Up to first 7 bytes is extra storage for sptr
    // - Length is stored as 2 * (MaxShortLen - Len)
    // ---- Bottom bit will always be zero in this case
    // ---- At max length (23 bytes), length acts as the NULL byte
    //
    // If heap allocation:
    // - This field is treated as the heap size for the allocation
    // - Assume all allocations are 8 byte aligned, therefore bottom
    //   bottom three bits can be used as flag bits
    // -- Bit 0 unused
    // -- Bit 1 unused
    // -- Bit 2 heap flag
    //
    // ;tldr
    //
    // - Stack: encoded length
    // - Heap:  capacity
    //
    union 
    {
        struct
        {
            u64 EncodedLen;
        } Stack;

        struct
        {
            u64 Capacity;
        } Heap;
    } mFooter;
};