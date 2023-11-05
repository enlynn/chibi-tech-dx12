#pragma once

#include <types.h>

#include "d3d12_common.h"

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

private:
	ID3D12Device*  mDevice                = nullptr;
	IDXGIAdapter1* mAdapter               = nullptr;
	u32            mSupportedFeatureLevel = 0;

	void EnableDebugDevice();
	void SelectAdapter();

	// TODO: requested and enabled features.
};