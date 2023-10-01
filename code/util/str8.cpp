#include "str8.h"
#include "bit.h"

#include <stdlib.h>
#include <string.h>
#include <cassert>
#include <stdarg.h>
#include <stdio.h>

#define STACK_STR_SIZE 23

#if (IS_LITTLE_ENDIAN==1)
#  define SMALL_STRING_MASK  0xFF00000000000000
#  define STRING_ZERO_MASK   0x00FFFFFFFFFFFFFF
#  define SMALL_STRING_SHIFT (u64)56
#  define HEAP_STRING_BIT    (u64)63
#else
#  define SMALL_STRING_MASK  0x00000000000000FF
#  define STRING_ZERO_MASK   0xFFFFFFFFFFFFFF00
#  define SMALL_STRING_SHIFT 0
#  define HEAP_STRING_BIT    (u64)0
#endif

// Wrapper for encoding/decoding an mstr8 length

constexpr u64
EncodeLength(u64 Length)
{
    u64 EncodedLength = 0;

    u64 Size      =   STACK_STR_SIZE - Length;
    Size          <<= 1;                         // Zero the bottom bit
    Size          <<= SMALL_STRING_SHIFT;        // Set length to bottom byte
    EncodedLength |=  Size;
    
    return EncodedLength;
}

constexpr u64
DecodeLength(u64 EncodedLength)
{
    u64 DecodedLength = 0;

    DecodedLength =   EncodedLength;
    DecodedLength &=  SMALL_STRING_MASK;              // Get the length byte
    DecodedLength >>= SMALL_STRING_SHIFT;
    DecodedLength >>= 1;                              // Divide by two
    DecodedLength =   STACK_STR_SIZE - DecodedLength; // Reverse the length (gets stored as STACK_STR_SIZE - length)
     
    return DecodedLength;
}

// Misc one-liners that have to be in the implementation section because they call
// strlen() or memcmp().

bool 
operator==(istr8 Lhs, istr8 Rhs) 
{ 
	return (Lhs.Length() == Rhs.Length() && memcmp((void*)Lhs.Ptr(), Rhs.Ptr(), Lhs.Length()) == 0); 
}

bool 
operator==(istr8 Lhs, const char* Rhs) 
{ 
	return (Lhs.Length() == strlen(Rhs) && memcmp((void*)Lhs.Ptr(), Rhs, Lhs.Length()) == 0);
}

bool 
operator==(const char* Lhs, istr8 Rhs) 
{ 
	return (strlen(Lhs) == Rhs.Length() && memcmp((void*)Lhs, Rhs.Ptr(), Rhs.Length()) == 0);
}

bool 
operator==(const mstr8& Lhs, const mstr8& Rhs) 
{ 
	return (Lhs.Length() == Rhs.Length() && memcmp((void*)Lhs.Ptr(), Rhs.Ptr(), Lhs.Length()) == 0);
}

bool 
operator==(const mstr8& Lhs, istr8 Rhs) 
{ 
	return (Lhs.Length() == Rhs.Length() && memcmp((void*)Lhs.Ptr(), Rhs.Ptr(), Lhs.Length()) == 0);
}

bool 
operator==(const mstr8& Lhs, const char* Rhs) 
{ 
	return (Lhs.Length() == strlen(Rhs) && memcmp((void*)Lhs.Ptr(), Rhs, Lhs.Length()) == 0);
}

bool 
operator==(istr8 Lhs, const mstr8& Rhs) 
{ 
	return (Lhs.Length() == Rhs.Length() && memcmp((void*)Lhs.Ptr(), Rhs.Ptr(), Lhs.Length()) == 0);
}

bool 
operator==(const char* Lhs, const mstr8& Rhs) 
{ 
	return (strlen(Lhs) == Rhs.Length() && memcmp((void*)Lhs, Rhs.Ptr(), Rhs.Length()) == 0);
}

istr8::istr8(const char* Ptr) : mBorrowedPtr(Ptr), mLen(strlen(Ptr)) {}
mstr8::mstr8(const char* Ptr) : mstr8(Ptr, strlen(Ptr)) {}

mstr8& mstr8::Insert(u64 Index, const char* Str) { return Insert(Index,    Str, (u64)strlen(Str)); }
mstr8& mstr8::Prepend(const char* Str)           { return Insert((u64)0,   Str, (u64)strlen(Str)); }
mstr8& mstr8::Append(const char* Str)            { return Insert(Length(), Str, (u64)strlen(Str)); }

//
// istr8 Implementation
//


//
// mstr8 Implementation
//

mstr8::mstr8()
{
    mData.Stack.Ptr[0]       = 0;
    mFooter.Stack.EncodedLen = EncodeLength(0);
}

mstr8::mstr8(const char* Ptr, u64 Length) : mstr8()
{
    assert(Ptr);

    // Make sure the embedded fields (if stack string) are zeroed
    mData.Heap.Length     = 0;
    mFooter.Heap.Capacity = 0;

    if (Length <= STACK_STR_SIZE)
    { // Reserve fits on the stack
        memcpy(mData.Stack.Ptr, Ptr, Length);
        mData.Stack.Ptr[Length] = 0; //intentional buffer overrun
        mFooter.Stack.EncodedLen |= EncodeLength(Length);
    }
    else
    { // Alloc string on the heap
        mData.Heap.Length = Length;
        // Make sure the capcity is always forward aligned to 8 bytes so that the 
        // bottom bits are always cleared.
        mFooter.Heap.Capacity = ForwardAlign(Length + 1, 8);

        mData.Heap.Ptr = (char*)malloc(mFooter.Heap.Capacity);
        memcpy(mData.Heap.Ptr, Ptr, Length);
        mData.Heap.Ptr[Length] = 0;

        // heap remains set to 0 to mark it as dirty set bottom bit to 1, to mark the heap
        mFooter.Heap.Capacity |= BitMask(HEAP_STRING_BIT);
    }
}

mstr8 mstr8::Format(const char* StrFormat, ...)
{
    assert(StrFormat);

    mstr8 Result = {};

    va_list Args;
    va_start(Args, StrFormat);

    va_list Copy;
    va_copy(Copy, Args);
    int Length = 1 + vsnprintf(nullptr, 0, StrFormat, Copy);
    va_end(Copy);

    Result.SetLength(Length);

    vsnprintf(Result.Ptr(), Length, StrFormat, Args);

    va_end(Args);

    return Result;
}

mstr8::mstr8(const mstr8& Other)
{
    u64 OtherLength = Other.Length();

    if (Other.IsHeap())
    {
        mData.Heap.Length = OtherLength;
        // Make sure the capcity is always forward aligned to 8 bytes so that the 
        // bottom bits are always cleared.
        mFooter.Heap.Capacity = ForwardAlign(mData.Heap.Length + 1, 8);

        mData.Heap.Ptr = (char*)malloc(mFooter.Heap.Capacity);
        memcpy(mData.Heap.Ptr, Other.Ptr(), mData.Heap.Length);
        mData.Heap.Ptr[mData.Heap.Length] = 0;

        // heap remains set to 0 to mark it as dirty set bottom bit to 1, to mark the heap
        mFooter.Heap.Capacity |= BitMask(HEAP_STRING_BIT);
    }
    else
    {
        memcpy(mData.Stack.Ptr, Other.Ptr(), OtherLength);
        mData.Stack.Ptr[OtherLength] = 0;
        mFooter.Stack.EncodedLen |= EncodeLength(OtherLength);
    }
}

mstr8::mstr8(mstr8&& Other)
{
    if (Other.IsHeap())
    {
        mData.Heap.Ptr        = Other.Ptr();
        mData.Heap.Length     = Other.Length();
        mFooter.Heap.Capacity = Other.Capacity();
    }
    else
    {
        u64 OtherLength = Other.Length();
        memcpy(mData.Stack.Ptr, Other.Ptr(), OtherLength);
        mData.Stack.Ptr[OtherLength] = 0;
        mFooter.Stack.EncodedLen = EncodeLength(OtherLength);
    }

    Other.mData.Heap.Ptr        = nullptr;
    Other.mData.Heap.Length     = 0;
    Other.mFooter.Heap.Capacity = 0;
}

mstr8& 
mstr8::operator=(const mstr8& Other)
{
    u64 OtherLength = Other.Length();

    if (Other.IsHeap())
    {
        mData.Heap.Length = OtherLength;
        // Make sure the capcity is always forward aligned to 8 bytes so that the 
        // bottom bits are always cleared.
        mFooter.Heap.Capacity = ForwardAlign(mData.Heap.Length + 1, 8);

        mData.Heap.Ptr = (char*)malloc(mFooter.Heap.Capacity);
        memcpy(mData.Heap.Ptr, Other.Ptr(), mData.Heap.Length);
        mData.Heap.Ptr[mData.Heap.Length] = 0;

        // heap remains set to 0 to mark it as dirty set bottom bit to 1, to mark the heap
        mFooter.Heap.Capacity |= BitMask(HEAP_STRING_BIT);
    }
    else
    {
        memcpy(mData.Stack.Ptr, Other.Ptr(), OtherLength);
        mData.Stack.Ptr[OtherLength] = 0;
        mFooter.Stack.EncodedLen |= EncodeLength(OtherLength);
    }

    return *this;
}

mstr8& 
mstr8::operator=(mstr8&& Other)
{
    if (Other.IsHeap())
    {
        mData.Heap.Ptr        = Other.Ptr();
        mData.Heap.Length     = Other.Length();
        mFooter.Heap.Capacity = Other.Capacity();
    }
    else
    {
        u64 OtherLength = Other.Length();
        memcpy(mData.Stack.Ptr, Other.Ptr(), OtherLength);
        mData.Stack.Ptr[OtherLength] = 0;
        mFooter.Stack.EncodedLen = EncodeLength(OtherLength);
    }

    Other.mData.Heap.Ptr        = nullptr;
    Other.mData.Heap.Length     = 0;
    Other.mFooter.Heap.Capacity = 0;

    return *this;
}

mstr8& 
mstr8::Insert(u64 Index, const char* Str, u64 StrLen)
{
    u64 OldLength = Length();
    assert(Str && Index <= OldLength);

    SetLength(OldLength + StrLen);

    if (Index < OldLength) memmove(Ptr() + Index + StrLen, Ptr() + Index, OldLength - Index);
    if (StrLen > 0)        memcpy(Ptr() + Index, Str, StrLen);

    return *this;
}

mstr8& 
mstr8::Remove(u64 Index, u64 Count)
{
    u64 OldLength = Length();
    assert(Index <= OldLength);

    u64 NewLength           = OldLength - Count;
    u64 NewLengthWithOffset = NewLength - Index;

    if (Index + Count < OldLength) memmove(Ptr() + Index, Ptr() + Index + Count, NewLengthWithOffset);
    SetLength(NewLength);

    return *this;
}

void 
mstr8::Free()
{
    if (IsHeap())
    {
        free(mData.Heap.Ptr);
    }

    mData.Heap.Ptr        = nullptr;
    mData.Heap.Length     = 0;
    mFooter.Heap.Capacity = 0;
}

void 
mstr8::SetLength(u64 RequestedLength)
{
    if (RequestedLength == Length()) return;
    ExpandIfNeeded(RequestedLength);

    if (IsHeap())
    {
        mData.Heap.Length = RequestedLength;
        mData.Heap.Ptr[RequestedLength] = 0;
    }
    else
    {
        mData.Stack.Ptr[RequestedLength] = 0;
        mFooter.Stack.EncodedLen = EncodeLength(RequestedLength);
    }
}

void 
mstr8::ExpandIfNeeded(u64 RequiredCapacity)
{
    u64 OldCapacity = Capacity();
    u64 OldLength   = Length();

    if (RequiredCapacity <= STACK_STR_SIZE || OldCapacity >= RequiredCapacity) return;

    // We'll double in size, or if that isn't enough we will just allocate exactly the required number of bytes.
    u64 NewCapacity = (OldCapacity * 2 > RequiredCapacity) ? OldCapacity * 2 : RequiredCapacity;
    NewCapacity = ForwardAlign(NewCapacity + 1, 8);

    if (IsHeap())
    { // Already on the heap, just realloc
        mData.Heap.Ptr = (char*)realloc(mData.Heap.Ptr, NewCapacity);
    }
    else
    {
        char* NewPtr = (char*)malloc(NewCapacity);
        memcpy(NewPtr, mData.Stack.Ptr, OldLength);

        mData.Heap.Ptr = NewPtr;
        mData.Heap.Ptr[OldLength] = 0;
    }

    // heap remains set to 0 to mark it as dirty set bottom bit to 1, to mark the heap
    mFooter.Heap.Capacity = NewCapacity;
    mFooter.Heap.Capacity |= BitMask(HEAP_STRING_BIT);
}

void 
mstr8::ShrinkToFit()
{
    if (!IsHeap()) return; // no need to shrink the stack container

    u64 CurrentLength = Length();
    if (CurrentLength <= STACK_STR_SIZE)
    { // Place the string on the stack
        char* OldStr = mData.Heap.Ptr;

        // Zero the embeded fields
        mData.Heap.Length     = 0;
        mFooter.Heap.Capacity = 0;

        memcpy(mData.Stack.Ptr, OldStr, CurrentLength);
        mData.Stack.Ptr[CurrentLength] = 0;
        mFooter.Stack.EncodedLen |= EncodeLength(CurrentLength);

        free(OldStr);
    }
    else
    {
        // Make sure the capcity is always forward aligned to 8 bytes so that the 
        // bottom bits are always cleared.
        mFooter.Heap.Capacity = ForwardAlign(CurrentLength + 1, 8);

        mData.Heap.Ptr = (char*)realloc(mData.Heap.Ptr, mFooter.Heap.Capacity);
        mData.Heap.Ptr[CurrentLength] = 0;

        // heap remains set to 0 to mark it as dirty set bottom bit to 1, to mark the heap
        mFooter.Heap.Capacity |= BitMask(HEAP_STRING_BIT);
    }
}

constexpr u64 
mstr8::Length() const
{
    u64 Result = IsHeap() ? mData.Heap.Length : DecodeLength(mFooter.Stack.EncodedLen);
    return Result;
}

constexpr u64  
mstr8::Capacity() const
{
    u64 Capacity = STACK_STR_SIZE;
    if (IsHeap())
    { // Clear the heap string bit
        Capacity = mFooter.Heap.Capacity;
        Capacity = BitFlip(Capacity, HEAP_STRING_BIT);
    }
    return Capacity;
}

constexpr bool 
mstr8::IsHeap() const
{
    return IsBitSet(mFooter.Heap.Capacity, HEAP_STRING_BIT);
}