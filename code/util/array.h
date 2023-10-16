#pragma once

//#include <types.h>
#include <util/allocator.h>

// TODO:
// - stack array (std::array): sarray

// fixed sized array, does not own memory. used for two situations:
// - Immutable Array, acts as a convienient wrapper around a Ptr and Length
// - Fixed Sized Array, must free separately with an allocator.
template<class T>
class farray
{
public:
	farray() = delete;
	constexpr farray(T* Array, u64 Count) : mArray(Array), mCount(Count) {}

	farray(const allocator& Allocator, u64 Count)
	{
		mArray = Allocator.AllocArray<T>(Count);
		mCount = Count;
	}

	~farray() { mArray = nullptr; mCount = 0; }

	void Deinit(const allocator& Allocator)
	{
		Allocator.FreeArray(mArray, mCount);
		mArray = nullptr;
		mCount = 0;
	}

	constexpr operator const T* ()           const { return mArray; }
	constexpr operator T* ()                       { return mArray; }

	constexpr u64      Length()              const { return mCount; }
	constexpr const T* Ptr()                 const { return mArray; }
	constexpr       T* Ptr()                       { return mArray; }

	constexpr const T& operator[](u64 Index) const { return Ptr()[Index];     }
	constexpr       T& operator[](u64 Index)       { return Ptr()[Index];     }

	// Legacy iterators
	constexpr const T* begin()               const { return Ptr();            }
	constexpr const T* end()                 const { return Ptr() + Length(); }
	constexpr       T* begin()                     { return Ptr();            }
	constexpr       T* end()                       { return Ptr() + Length(); }

	// Comparison operators. Comparison with marray is implemented inside of marray.
	// Requires the == operator to be implemented for the underlying type.
	inline friend bool operator==(farray Lhs, farray Rhs)
	{
		u64 LeftLength  = Lhs.Length();
		u64 RightLength = Rhs.Length();
		if (LeftLength != RightLength) return false;

		ForRange(u64, i, LeftLength)
		{
			if (Lhs[i] != Rhs[i]) return false;
		}

		return true;
	}

	inline friend bool operator!=(farray Lhs, farray Rhs)  { return !(Lhs == Rhs); }

private:
	T*  mArray = nullptr;
	u64 mCount = 0;
};

// mutable array, owns memory and will self-deconstruct
// TODO: Allow for a sentinal value
template<class T>
class darray
{
public:
	darray() = default;

	// Copies an array to a new block of memory. Relies of T having implemented the copy operator.
	explicit darray(const allocator& Allocator, T* OtherArray, u64 Count)
	{
		mAllocator = Allocator.Clone();
		mCapacity  = Count;
		mCount     = Count;

		mArray = Allocator.AllocArray<T>(Count, allocation_strategy::none);
		ForRange(u64, i, mCount)
		{
			mArray[i] = OtherArray[i];
		}
	}

	// Create an array with the specified capacity.
	explicit darray(const allocator& Allocator, u64 Capacity)
	{
		mAllocator = Allocator.Clone();
		mCapacity  = Capacity;
		mCount     = 0;
		mArray     = Allocator.AllocArray<T>(Capacity , allocation_strategy::default_init);
	}

	void Reset()
	{
		ForRange(u64, i, mCount)
		{
			mArray[i].~T();
		}

		mCount = 0;
	}

	void Clear()                                   { Reset(); ShrinkToFit(); } // Clears the array and frees the memory
	~darray() { Clear(); }

	// TODO: Copy and Move and Clone

	// Dereference functions
	constexpr operator const T* ()           const { return mArray; }
	constexpr operator       T* ()                 { return mArray; }

	constexpr u64      Length()              const { return mCount; }
	constexpr const T* Ptr()                 const { return mArray; }
	constexpr       T* Ptr()                       { return mArray; }

	// Insert functions
	void Insert(T& Element, u64 Index)
	{
		assert(Index <= mCount);
		assert(Index >= 0);
		ExpandIfNeeded(mCount + 1);
		
		if (Index < mCount)
		{
			for (u64 i = Index + 1; i <= mCount; ++i)
			{
				mArray[i] = mArray[i - 1];
			}
		}

		mArray[Index] = Element;
		mCount       += 1;
	}

	inline void PushFront(T& Element)              { Insert(Element, mCount); }
	inline void PushBack(T& Element)               { Insert(Element, 0);      }

	// Remove functions
	void Remove(u64 Index)
	{
		assert(Index < mCount); // catches count = 0
		mArray[Index].~T();

		for (u64 i = Index; i < mCount - 1; ++i) 
		{
			mArray[i] = mArray[i + 1];
		}

		mCount -= 1;
	}

	// Remove the element at Index and swap with the last element in the list
	void RemoveAndSwap(u64 Index)
	{
		assert(Index < mCount); // catches count = 0
		mArray[Index].~T();
		
		if (Index < mCount - 1)
		{ // Don't swap if this is the final element
			mArray[Index] = mArray[mCount - 1];
		}

		mCount -= 1;
	}
	
	// Remove from the end of the list
	inline void PopBack()                 { if (mCount > 0) Remove(mCount - 1); } 
	inline void PopFront()                { Remove(0);                          }

	// Shrink the capacity to fit the number of elements in the array
	void ShrinkToFit()
	{
		if (mCapacity == 0) return;

		if (mCount == 0)
		{
			mAllocator.FreeArray(mArray, mCapacity, allocation_strategy::deconstruct); 
			mArray    = nullptr;
			mCapacity = 0;
		}
		else
		{
			u64 NewCapacity = mCount;
			T*  NewArray    = mAllocator.AllocArray<T>(NewCapacity);
			ForRange(u64, i, mCount)
			{
				NewArray[i] = mArray[i];
			}

			// NOTE(enlynn): I am not entirely sure what the correct approach here should be. This 
			// assumes the copy operator above will correctly deconstruct the old element, but I think
			// this assumption might be a bit of a stretch. We will have to see in the long run...
			mAllocator.FreeArray(mArray, mCapacity, allocation_strategy::none);

			mArray    = NewArray;
			mCapacity = NewCapacity;
		}
	}

	constexpr const T& operator[](u64 Index) const { return Ptr()[Index]; }
	constexpr       T& operator[](u64 Index)       { return Ptr()[Index]; }

	// Legacy iterators
	constexpr const T* begin()               const { return Ptr();            }
	constexpr const T* end()                 const { return Ptr() + Length(); }
	constexpr       T* begin()                     { return Ptr();            }
	constexpr       T* end()                       { return Ptr() + Length(); }

	// Comparison operators. Comparison with marray is implemented inside of marray.
	inline friend bool operator==(const darray<T>& Lhs, const darray<T>& Rhs)
	{
		return CompareArrays(Lhs.Ptr(), Lhs.Length(), Rhs.Ptr(), Rhs.Length());
	}

	inline friend bool operator==(const farray<T>& Lhs, const darray<T>& Rhs)
	{
		return CompareArrays(Lhs.Ptr(), Lhs.Length(), Rhs.Ptr(), Rhs.Length());
	}

	inline friend bool operator==(const darray<T>& Lhs, const farray<T>& Rhs)
	{
		return CompareArrays(Lhs.Ptr(), Lhs.Length(), Rhs.Ptr(), Rhs.Length());
	}

	inline friend bool operator!=(const darray<T>& Lhs, const darray<T>& Rhs) { return !(Lhs == Rhs); }
	inline friend bool operator!=(const farray<T>& Lhs, const darray<T>& Rhs) { return !(Lhs == Rhs); }
	inline friend bool operator!=(const darray<T>& Lhs, const farray<T>& Rhs) { return !(Lhs == Rhs); }

	darray(const darray& Other)
		: mAllocator(Other.mAllocator.Clone())
		, mArray(nullptr)
		, mCount(Other.mCount)
		, mCapacity(Other.mCapacity)
	{
		if (mCapacity > 0)
		{
			mArray = mAllocator.AllocArray<T>(mCapacity, allocation_strategy::none);
			ForRange(u64, i, mCount)
			{
				mArray[i] = Other.mArray[i];
			}
		}
	}

	darray(darray&& Other)
		: mAllocator(Other.mAllocator.Clone())
		, mArray(Other.mArray)
		, mCount(Other.mCount)
		, mCapacity(Other.mCapacity)
	{
		Other.mAllocator = {};
		Other.mCount     = 0;
		Other.mCapacity  = 0;
		Other.mArray     = nullptr;
	}

	darray& operator=(const darray& Other)
	{
		mAllocator = Other.mAllocator.Clone();
		mCount     = Other.mCount;
		mCapacity  = Other.mCapacity;
		mArray     = nullptr;

		if (mCapacity > 0)
		{
			mArray = mAllocator.AllocArray<T>(mCapacity, allocation_strategy::none);
			ForRange(u64, i, mCount)
			{
				mArray[i] = Other.mArray[i];
			}
		}

		return *this;
	}

	darray& operator=(darray&& Other)
	{
		mAllocator = Other.mAllocator.Clone();
		mCount     = Other.mCount;
		mCapacity  = Other.mCapacity;
		mArray     = Other.mArray;

		Other.mAllocator = {};
		Other.mCount     = 0;
		Other.mCapacity  = 0;
		Other.mArray     = nullptr;

		return *this;
	}

private:

	allocator mAllocator = {};
	T*        mArray     = nullptr;
	u64       mCount     = 0;
	u64       mCapacity  = 0;

	constexpr bool CompareArrays(const T* Left, const u64 LeftLength, const T* Right, const u64 RightLength)
	{
		if (LeftLength != RightLength) return false;

		ForRange(u64, i, LeftLength)
		{
			if (Left[i] != Right[i]) return false;
		}

		return true;
	}

	void ExpandIfNeeded(u64 RequiredCapacity)
	{
		if (RequiredCapacity <= mCapacity) return;

		u64 OldCapacity = mCapacity;
		u64 NewCapacity = (OldCapacity * 2 > RequiredCapacity) ? OldCapacity * 2 : RequiredCapacity;
		if (NewCapacity == 0) NewCapacity = 5;

		T* NewArray = mAllocator.AllocArray<T>(NewCapacity);
		ForRange(u64, i, mCount)
		{
			NewArray[i] = mArray[i];
		}

		// NOTE(enlynn): I am not entirely sure what the correct approach here should be. This 
		// assumes the copy operator above will correctly deconstruct the old element, but I think
		// this assumption might be a bit of a stretch. We will have to see in the long run...
		mAllocator.FreeArray(mArray, mCapacity, allocation_strategy::none);

		mArray    = NewArray;
		mCapacity = NewCapacity;
	}
};
