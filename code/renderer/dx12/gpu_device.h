#pragma once

#include <types.h>

#include "d3d12_common.h"
#include "gpu_resource.h"

struct commited_resource_info
{
    D3D12_HEAP_TYPE          HeapType      = D3D12_HEAP_TYPE_DEFAULT;
    u64                      Size          = 0;
    u64                      Alignment     = 0;
    D3D12_RESOURCE_FLAGS     ResourceFlags = D3D12_RESOURCE_FLAG_NONE;
    D3D12_HEAP_FLAGS         HeapFlags     = D3D12_HEAP_FLAG_NONE;
    D3D12_RESOURCE_STATES    InitialState  = D3D12_RESOURCE_STATE_COMMON;
    const D3D12_CLEAR_VALUE* ClearValue    = nullptr;
};

class gpu_device
{
public:
	gpu_device() = default;
	void Init();

	void Deinit();
	~gpu_device() { Deinit(); }

	void ReportLiveObjects();

	// Cast to the internal ID3D12Device handle
	constexpr ID3D12Device*  AsHandle()  const { return mDevice;  }
	constexpr IDXGIAdapter1* AsAdapter() const { return mAdapter; }

	ID3D12DescriptorHeap* CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type, u32 Count, bool IsShaderVisible);

    gpu_resource          CreateCommittedResource(commited_resource_info& Info);

private:
	ID3D12Device*  mDevice                = nullptr;
	IDXGIAdapter1* mAdapter               = nullptr;
	u32            mSupportedFeatureLevel = 0;

	void EnableDebugDevice();
	void SelectAdapter();

	// TODO: requested and enabled features.
};
