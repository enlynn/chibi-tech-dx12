#pragma once

#include <types.h>
#include "d3d12_common.h"

#include <util/array.h>

class gfx_device;

struct gpu_descriptor_allocation
{
};

class gpu_descriptor_allocator
{
public:
    gpu_descriptor_allocator(gfx_device* Device, const allocator& Allocator, D3D12_DESCRIPTOR_HEAP_TYPE Type, u32 CountPerHeap = 256);


private:
    // Describes the type of descriptors that can be staged using this
    // dynamic descriptor heap. Valid values are:
    // - D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
    // - D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
    D3D12_DESCRIPTOR_HEAP_TYPE           mHeapType           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    u32                                  mDescriptorsPerHeap = 256;
    u32                                  mDescriptorStride   = 0;

    // Move this to the allocation type?
    farray<D3D12_CPU_DESCRIPTOR_HANDLE>  mCpuHandleCache; // Capacity == mDescriptorsPerHeap

    // List of allocated descriptor heap pool. Acts as a backing storage
    // for Reset calls. On Reset, the heap pool is copied over to
    // avail_descriptors
    darray<ID3D12DescriptorHeap*>        mDescriptorHeapList = {};
    u32                                  mNextAvailableHeap  = 0;

    // Current descriptor heap bound to the command list
    ID3D12DescriptorHeap*                mCurrentHeap      = nullptr;
    D3D12_GPU_DESCRIPTOR_HANDLE          mCurrentGpuHandle = { .ptr = 0 };
    D3D12_CPU_DESCRIPTOR_HANDLE          mCurrentCpuHandle = { .ptr = 0 };
    u32                                  mNumFreeHandles   = 0;
};