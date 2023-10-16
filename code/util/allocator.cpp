#include "allocator.h"

#include <string.h>

void* AllocatorInterfaceAllocStub(void* _Self, u64 _Size)                 { assert(false); return nullptr; }
void* AllocatorInterfaceReallocStub(void* _Self, void* _InPtr, u64 _Size) { assert(false); return nullptr; }
void  AllocatorInterfaceFreeStub(void* _Self, void* _Ptr)                 {}

fn_internal void* MallocWrapper(void* _Self, u64 Size)               { return malloc(Size);         }
fn_internal void* ReallocWrapper(void* _Self, void* InPtr, u64 Size) { return realloc(InPtr, Size); }
fn_internal void  FreeWrapper(void* _Self, void* Ptr)                { free(Ptr);                   }

allocator::allocator(allocator_interface Interface, allocator_hint Hint)
	: mInterface(Interface)
	, mHint(Hint)
{
}

allocator 
allocator::Default(allocator_hint Hint)
{
	allocator_interface Interface = {
		.Alloc   = MallocWrapper,
		.Realloc = ReallocWrapper,
		.Free    = FreeWrapper,
		.Self    = nullptr,
	};

	return allocator(Interface, Hint);
}

allocator 
allocator::Clone() const
{
	allocator Result = allocator(mInterface, mHint);
	return Result;
}

void allocator::ZeroMemoryBlock(void* Ptr, u64 Size) const
{
	memset(Ptr, 0, Size);
}

void
allocator::CopyMemoryBlock(u8* Dst, u64 DstSize, u8* Src) const
{
	memmove(Dst, Src, DstSize);
}