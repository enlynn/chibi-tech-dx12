#include "gfx_dynamic_descriptor_heap.h"

#include "gfx_device.h"
#include "gfx_root_signature.h"
#include "gfx_command_list.h"

#include <util/bit.h>

fn_internal void
SetRootDescriptorGraphicsWrapper(ID3D12GraphicsCommandList* CommandList, UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE GpuDescriptorHandle)
{
	CommandList->SetGraphicsRootDescriptorTable(RootIndex, GpuDescriptorHandle);
}

fn_internal void
SetRootDescriptorComputeWrapper(ID3D12GraphicsCommandList* CommandList, UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE GpuDescriptorHandle)
{
	CommandList->SetComputeRootDescriptorTable(RootIndex, GpuDescriptorHandle);
}

fn_internal void
SetGraphicsRootCBVWrapper(ID3D12GraphicsCommandList* CommandList, UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS GpuDescriptorHandle)
{
	CommandList->SetGraphicsRootConstantBufferView(RootIndex, GpuDescriptorHandle);
}

fn_internal void
SetGraphicsRootSRVWrapper(ID3D12GraphicsCommandList* CommandList, UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS GpuDescriptorHandle)
{
	CommandList->SetGraphicsRootShaderResourceView(RootIndex, GpuDescriptorHandle);
}

fn_internal void
SetGraphicsRootUAVWrapper(ID3D12GraphicsCommandList* CommandList, UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS GpuDescriptorHandle)
{
	CommandList->SetGraphicsRootUnorderedAccessView(RootIndex, GpuDescriptorHandle);
}

fn_internal void
SetComputeRootCBVWrapper(ID3D12GraphicsCommandList* CommandList, UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS GpuDescriptorHandle)
{
	CommandList->SetComputeRootConstantBufferView(RootIndex, GpuDescriptorHandle);
}

fn_internal void
SetComputeRootSRVWrapper(ID3D12GraphicsCommandList* CommandList, UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS GpuDescriptorHandle)
{
	CommandList->SetComputeRootShaderResourceView(RootIndex, GpuDescriptorHandle);
}

fn_internal void
SetComputeRootUAVWrapper(ID3D12GraphicsCommandList* CommandList, UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS GpuDescriptorHandle)
{
	CommandList->SetComputeRootUnorderedAccessView(RootIndex, GpuDescriptorHandle);
}


gfx_dynamic_descriptor_heap::gfx_dynamic_descriptor_heap(gfx_device* Device, const allocator& Allocator, dynamic_heap_type Type, u32 CountPerHeap)
{
	mDevice             = Device;
	mAllocator          = Allocator.Clone();
	mDescriptorsPerHeap = CountPerHeap;
	mDescriptorStride   = mDevice->AsHandle()->GetDescriptorHandleIncrementSize(mHeapType);

	switch (Type)
	{
		case dynamic_heap_type::buffer:  mHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		case dynamic_heap_type::sampler: mHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	}

	mCpuHandleCache     = mAllocator.AllocArray<D3D12_CPU_DESCRIPTOR_HANDLE>(mDescriptorsPerHeap, allocation_strategy::zero);
	mDescriptorHeapList = darray<ID3D12DescriptorHeap*>(mAllocator, 1);
}

void 
gfx_dynamic_descriptor_heap::Deinit()
{
	mAllocator.FreeArray(mCpuHandleCache, mDescriptorsPerHeap);
	mCpuHandleCache = nullptr;

	ForRange(u64, i, mDescriptorHeapList.Length())
		ComSafeRelease(mDescriptorHeapList[i]);
	mDescriptorHeapList.Clear();

	mDevice = nullptr;
}

void
gfx_dynamic_descriptor_heap::Reset()
{
	mNextAvailableHeap            = 0;
	mCurrentHeap                  = nullptr;
	mCurrentCpuHandle             = {};
	mCurrentGpuHandle             = {};
	mNumFreeHandles               = 0;
	mStaleDescriptorTableBitmask  = 0;
	mCachedDescriptorTableBitmask = 0;
	mStaleCbvBitmask              = 0;
	mStaleSrvBitmask              = 0;
	mStaleUavBitmask              = 0;

	// Reset the table cache
	ForRange (int, i, cMaxDescriptorTables)
	{
		mDescriptorTableCache[i].mNumDescriptors = 0;
		mDescriptorTableCache[i].mBaseDescriptor = nullptr;
	}

	ForRange(int, i, cMaxInlineDescriptors)
	{
		mInlineCbv[i] = 0ull;
		mInlineSrv[i] = 0ull;
		mInlineUav[i] = 0ull;
	}
}

void
gfx_dynamic_descriptor_heap::ParseRootSignature(const gfx_root_signature& RootSignature)
{
	const u32 RootParameterCount = RootSignature.GetRootParameterCount();

	// if the root sig changes, must rebind all descriptors to the command list
	mStaleDescriptorTableBitmask = 0;

	// Update the Descriptor Table Mask
	mCachedDescriptorTableBitmask = RootSignature.GetDescriptorTableBitmask(gfx_descriptor_type::cbv);
	u64 TableBitmask              = mCachedDescriptorTableBitmask;

	u32 CurrentDescriptorOffset = 0;
	DWORD RootIndex;
	while (_BitScanForward64(&RootIndex, TableBitmask) && RootIndex < RootParameterCount)
	{
		u32 NumDescriptors = RootSignature.GetNumDescriptors(RootIndex);
		assert(CurrentDescriptorOffset <= mDescriptorsPerHeap && "Root signature requires more than the max number of descriptors per descriptor heap. Consider enlaring the maximum.");

		// Pre-Allocate the CPU Descriptor Ranges needed - overwriting the CPU Cache is safe because it is 
		// the staging descriptor to eventually be copied to the GPU descriptor.
		descriptor_table_cache* TableCache = &mDescriptorTableCache[RootIndex];
		TableCache->mNumDescriptors = NumDescriptors;
		TableCache->mBaseDescriptor = mCpuHandleCache + CurrentDescriptorOffset;

		CurrentDescriptorOffset += NumDescriptors;

		// Flip the descirptor table so it's not scanned again
		TableBitmask = BitFlip(TableBitmask, (u64)RootIndex);
	}
}

void 
gfx_dynamic_descriptor_heap::StageDescriptors(u32 RootParameterIndex, u32 DescriptorOffset, u32 NumDescriptors, D3D12_CPU_DESCRIPTOR_HANDLE CpuDescriptorHandle)
{
	// Cannot stage more than the maximum number od descriptors per heap
	// Cannot stage more than MaxDescriptorTables root params
	assert(NumDescriptors <= mDescriptorsPerHeap || RootParameterIndex < cMaxDescriptorTables);

	// Check number of descriptors to copy does not exceed the number of descriptors expected in the descriptor table
	descriptor_table_cache* TableCache = &mDescriptorTableCache[RootParameterIndex];
	assert((DescriptorOffset + NumDescriptors) <= TableCache->mNumDescriptors);

	// Copy the source CPU Descriptor into the Descriptor Table's CPU Descriptor Cache
	D3D12_CPU_DESCRIPTOR_HANDLE* CpuDescriptors = (TableCache->mBaseDescriptor + DescriptorOffset);
	for (u32 DescriptorIndex = 0; DescriptorIndex < NumDescriptors; ++DescriptorIndex)
	{
		CpuDescriptors[DescriptorIndex].ptr = CpuDescriptorHandle.ptr + DescriptorIndex * mDescriptorStride;
	}

	// Mark the root parameter for update
	mStaleDescriptorTableBitmask = BitSet(mStaleDescriptorTableBitmask, u64(RootParameterIndex));
}

void 
gfx_dynamic_descriptor_heap::CommitDescriptorTables(gfx_command_list* CommandList, commit_descriptor_table_pfn CommitTablesPFN)
{
	// Compute the number of descriptors that need to be updated 
	u32 NumDescriptorsToCommit = ComputeStaleDescriptorTableCount();
	if (NumDescriptorsToCommit == 0) return; // If no descriptors have been updated, don't do anything

	// Update the active descriptor heap based on the number of descriptors we want to commit
	UpdateCurrentHeap(CommandList, NumDescriptorsToCommit);
	
	// Scan for the descriptor tables that have been updated, and only copy the new descriptors.
	// If this is a new heap, all descriptors are updated since they have to be on the same heap.
	DWORD RootIndex;
	while (_BitScanForward64(&RootIndex, mStaleDescriptorTableBitmask))
	{
		UINT                         DescriptorCount      = mDescriptorTableCache[RootIndex].mNumDescriptors;
		D3D12_CPU_DESCRIPTOR_HANDLE* SrcDescriptorHandles = mDescriptorTableCache[RootIndex].mBaseDescriptor;

		// TODO(enlynn): Would it be possible to update all stale descriptors at once?
		D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptorRangeStarts[] = { mCurrentCpuHandle };
		UINT                        DestDescriptorRangeSizes[]  = { DescriptorCount   };

		// Copy the staged CPU visible descriptors to the GPU visible descriptor heap.
		mDevice->AsHandle()->CopyDescriptors(1, DestDescriptorRangeStarts, DestDescriptorRangeSizes,
			DescriptorCount, SrcDescriptorHandles, nullptr, mHeapType);

		// Set the descriptors on the command list using the passed-in setter function.
		CommitTablesPFN(CommandList->AsHandle(), RootIndex, mCurrentGpuHandle);

		// Offset current CPU and GPU descriptor handles.
		mCurrentCpuHandle.ptr = mCurrentCpuHandle.ptr + u64(DescriptorCount * mDescriptorStride);
		mCurrentGpuHandle.ptr = mCurrentGpuHandle.ptr + u64(DescriptorCount * mDescriptorStride);
		mNumFreeHandles      -= DescriptorCount;

		// Flip the stale bit so the descriptor table is not recopied until the descriptor is updated
		mStaleDescriptorTableBitmask = BitFlip(mStaleDescriptorTableBitmask, u64(RootIndex));
	}
}

void 
gfx_dynamic_descriptor_heap::CommitInlineDescriptors(
	gfx_command_list*            CommandList,
	D3D12_GPU_VIRTUAL_ADDRESS*   GpuDescriptorHandles,
	u32*                         DescriptorBitmask,
	commit_descriptor_inline_pfn CommitInlinePFN)
{
	u32 Bitmask = *DescriptorBitmask;
	if (Bitmask == 0) return; // Don't do anything if no descriptors have been updated
	
	DWORD RootIndex;
	while (_BitScanForward(&RootIndex, Bitmask))
	{
		CommitInlinePFN(CommandList->AsHandle(), RootIndex, GpuDescriptorHandles[RootIndex]);
		// Flip the stale bit so the descriptor is not recopied until updated again
		Bitmask = BitFlip(Bitmask, (u32)RootIndex);
	}

	*DescriptorBitmask = Bitmask;
}

void 
gfx_dynamic_descriptor_heap::CommitStagedDescriptorsForDraw(gfx_command_list* CommandList)
{
	CommitDescriptorTables (CommandList,                                &SetRootDescriptorGraphicsWrapper);
	CommitInlineDescriptors(CommandList, mInlineCbv, &mStaleCbvBitmask, &SetGraphicsRootCBVWrapper);
	CommitInlineDescriptors(CommandList, mInlineSrv, &mStaleSrvBitmask, &SetGraphicsRootSRVWrapper);
	CommitInlineDescriptors(CommandList, mInlineUav, &mStaleUavBitmask, &SetGraphicsRootUAVWrapper);
}

void 
gfx_dynamic_descriptor_heap::CommitStagedDescriptorsForDispatch(gfx_command_list* CommandList)
{
	CommitDescriptorTables (CommandList,                                &SetRootDescriptorGraphicsWrapper);
	CommitInlineDescriptors(CommandList, mInlineCbv, &mStaleCbvBitmask, &SetComputeRootCBVWrapper);
	CommitInlineDescriptors(CommandList, mInlineSrv, &mStaleSrvBitmask, &SetComputeRootSRVWrapper);
	CommitInlineDescriptors(CommandList, mInlineUav, &mStaleUavBitmask, &SetComputeRootUAVWrapper);
}

D3D12_GPU_DESCRIPTOR_HANDLE 
gfx_dynamic_descriptor_heap::CopyDescriptor(gfx_command_list* CommandList, D3D12_CPU_DESCRIPTOR_HANDLE CpuDescriptorHandle)
{
	const u32 DescriptorsToUpdate = 1;

	// Update the active descriptor heap based on the number of descriptors we want to commit
	UpdateCurrentHeap(CommandList, DescriptorsToUpdate);

	D3D12_GPU_DESCRIPTOR_HANDLE Result = mCurrentGpuHandle;

	// Copy the Source CPU Descriptor Handle into the CPU Descriptor Cache
	mDevice->AsHandle()->CopyDescriptorsSimple(1, mCurrentCpuHandle, CpuDescriptorHandle, mHeapType);

	// Offset current CPU and GPU descriptor handles.
	mCurrentCpuHandle.ptr = mCurrentCpuHandle.ptr + u64(DescriptorsToUpdate * mDescriptorStride);
	mCurrentGpuHandle.ptr = mCurrentGpuHandle.ptr + u64(DescriptorsToUpdate * mDescriptorStride);
	mNumFreeHandles -= DescriptorsToUpdate;

	return Result;
}

void
gfx_dynamic_descriptor_heap::UpdateCurrentHeap(gfx_command_list* CommandList, u32 NumDescriptorsToCommit)
{
	if (!mCurrentHeap || mNumFreeHandles < NumDescriptorsToCommit)
	{
		// Not enough space to commit the requested descriptor count. Acquire a new heap to fullfil the request.
		mCurrentHeap      = RequestDescriptorHeap();
		mCurrentCpuHandle = mCurrentHeap->GetCPUDescriptorHandleForHeapStart();
		mCurrentGpuHandle = mCurrentHeap->GetGPUDescriptorHandleForHeapStart();
		mNumFreeHandles   = mDescriptorsPerHeap;

		// Update the current heap bound to the command list
		CommandList->SetDescriptorHeap(mHeapType, mCurrentHeap);

		// When updating the descriptor heap on the command list, all descriptor tables must be (re)recopied to 
		// the new descriptor heap (not just the stale descriptor tables).
		mStaleDescriptorTableBitmask = mCachedDescriptorTableBitmask;
	}
}

ID3D12DescriptorHeap* 
gfx_dynamic_descriptor_heap::RequestDescriptorHeap()
{
	ID3D12DescriptorHeap* Result = 0;

	if (mNextAvailableHeap < mDescriptorHeapList.Length())
	{
		Result = mDescriptorHeapList[mNextAvailableHeap];
		mNextAvailableHeap += 1;
	}
	else
	{
		Result = mDevice->CreateDescriptorHeap(mHeapType, mDescriptorsPerHeap, true);
		mDescriptorHeapList.PushBack(Result);
	}

	return Result;
}

u32 
gfx_dynamic_descriptor_heap::ComputeStaleDescriptorTableCount()
{
	u32 StaleDescriptorCount = 0;
	
	DWORD BitIndex;
	u64   Bitmask = mStaleDescriptorTableBitmask;

	// Count the number of bits set to 1
	while (_BitScanForward64(&BitIndex, Bitmask))
	{
		StaleDescriptorCount += mDescriptorTableCache[BitIndex].mNumDescriptors;
		Bitmask = BitFlip(Bitmask, (u64)BitIndex);
	}

	return StaleDescriptorCount;
}

// Stage inline descriptors
void 
gfx_dynamic_descriptor_heap::StageInlineCBV(u32 RootIndex, D3D12_GPU_VIRTUAL_ADDRESS GpuDescriptorHandle)
{
	assert(RootIndex < cMaxInlineDescriptors);
	mInlineCbv[RootIndex] = GpuDescriptorHandle;
	mStaleCbvBitmask      = BitSet(mStaleCbvBitmask, RootIndex);
}

void 
gfx_dynamic_descriptor_heap::StageInlineSRV(u32 RootIndex, D3D12_GPU_VIRTUAL_ADDRESS GpuDescriptorHandle)
{
	assert(RootIndex < cMaxInlineDescriptors);
	mInlineSrv[RootIndex] = GpuDescriptorHandle;
	mStaleSrvBitmask      = BitSet(mStaleSrvBitmask, RootIndex);
}

void 
gfx_dynamic_descriptor_heap::StageInlineUAV(u32 RootIndex, D3D12_GPU_VIRTUAL_ADDRESS GpuDescriptorHandle)
{
	assert(RootIndex < cMaxInlineDescriptors);
	mInlineUav[RootIndex] = GpuDescriptorHandle;
	mStaleUavBitmask      = BitSet(mStaleUavBitmask, RootIndex);
}