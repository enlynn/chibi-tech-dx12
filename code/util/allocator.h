#pragma once

#include <new> //placement new

#include <types.h>

// Default stubs for the allocator interface. This is done so that
// as long as the interface has been default initialized we won't
// crash if a user forgets to set a member.
void* AllocatorInterfaceAllocStub(void* Self, u64 Size);
void* AllocatorInterfaceReallocStub(void* Self, void* InPtr, u64 Size);
void  AllocatorInterfaceFreeStub(void* Self, void* Ptr);

// A "hint" users can provide to the allocator so that functions/classes
// can guarantee the allocator is the correct type.
enum class allocator_hint : u8
{
	none,    //should be default constructed allocator_interface. This is to detect invalid states
	general,
	pool,
	arena,
	unknown, //allocator that doesn't fit into the above categories. TODO: "custom" allocator values
};

enum class allocation_strategy : u8
{
	none,
	zero,          // zero the memory being allocated
	default_init,  // default initializes the memory
	deconstruct,   // when freeing, will call the deconstructor
};

struct allocator_interface
{
	void* (*Alloc)(void* Self, u64 Size)                = AllocatorInterfaceAllocStub;
	void* (*Realloc)(void* Self, void* InPtr, u64 Size) = AllocatorInterfaceReallocStub;
	void  (*Free)(void* Self, void* Ptr)                = AllocatorInterfaceFreeStub;
	void* Self                                          = nullptr; 
};

class allocator
{
public:
	allocator() = default;
	allocator(allocator_interface Interface, allocator_hint Hint = allocator_hint::general);

	// Retrieves the defail C Allocator using malloc, realloc, and free
	static allocator Default(allocator_hint Hint = allocator_hint::general);

	allocator Clone() const;

	// Helper Functions that have to call the standard library.
	void ZeroMemoryBlock(void* Ptr, u64 Size) const;
	void CopyMemoryBlock(u8* Dst, u64 DstSize, u8* Src) const; // Memory can overlap

	template<class T> T* Alloc(allocation_strategy Strategy = allocation_strategy::none)
	{
		void* Memory = mInterface.Alloc(mInterface.Self, sizeof(T));
		T* Result = nullptr;

		if (Strategy == allocation_strategy::zero)
		{
			ZeroMemoryBlock(Memory, sizeof(T));
			Result = (T*)Memory;
		}
		else if (Strategy == allocation_strategy::default_init)
		{
			Result = new (Memory) T();
		}
		else
		{
			Result = (T*)Memory;
		}

		return Result;
	}

	// Same as Alloc, but will call the constructor for the type
	template<class T, typename... Args> T* AllocEmplace(Args&&... args)
	{
		void* Memory = mInterface.Alloc(mInterface.Self, sizeof(T));
		return new (Memory) T(std::forward<Args>(args)...);
	}

	void* AllocChunk(u64 Size, allocation_strategy Strategy = allocation_strategy::none)
	{
		void* Result = mInterface.Alloc(mInterface.Self, Size);
		if (Strategy == allocation_strategy::zero)
		{
			ZeroMemoryBlock(Result, Size);
		}

		return Result;
	}

	template<class T> T* AllocArray(u64 Count, allocation_strategy Strategy = allocation_strategy::none) const
	{
		u64 Size = sizeof(T) * Count;
		void* Memory = mInterface.Alloc(mInterface.Self, Size);
		T* Result = nullptr;

		if (Strategy == allocation_strategy::zero)
		{
			ZeroMemoryBlock(Memory, Size);
			Result = (T*)Memory;
		}
		else if (Strategy == allocation_strategy::default_init)
		{
			Result = (T*)Memory;
			ForRange(u32, i, Count)
			{
				new (Result + i) T();
			}
		}
		else
		{
			Result = (T*)Memory;
		}

		return Result;
	}

	template<class T> T* Realloc(void* OldPtr, u64 OldCount = 1)
	{
		return (T*)mInterface.Realloc(mInterface.Self, OldPtr, OldCount);
	}

	template<class T> void Free(T* Ptr, allocation_strategy Strategy = allocation_strategy::none)
	{
		if (Strategy == allocation_strategy::deconstruct)
		{
			Ptr->~T();
		}

		mInterface.Free(mInterface.Self, (void*)Ptr);
	}

	template<class T> void FreeArray(T* Ptr, u64 Count, allocation_strategy Strategy = allocation_strategy::none) const
	{
		if (Strategy == allocation_strategy::deconstruct)
		{
			ForRange(u32, i, Count)
			{
				(Ptr + i)->~T();
			}
		}

		mInterface.Free(mInterface.Self, (void*)Ptr);
	}

private:
	allocator_hint      mHint      = allocator_hint::none;
	allocator_interface mInterface = {};
};

