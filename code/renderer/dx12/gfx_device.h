#pragma once

#include <types.h>

struct ID3D12Device;
struct IDXGIAdapter1;

class gfx_device
{
public:
	gfx_device() = default;
	void Init();

	void Deinit();
	~gfx_device() { Deinit(); }

	void ReportLiveObjects();

	// Cast to the internal ID3D12Device handle
	constexpr ID3D12Device*  AsHandle()  const { return mDevice;  }
	constexpr IDXGIAdapter1* AsAdapter() const { return mAdapter; }

private:
	ID3D12Device*  mDevice                = nullptr;
	IDXGIAdapter1* mAdapter               = nullptr;
	u32            mSupportedFeatureLevel = 0;

	void EnableDebugDevice();
	void SelectAdapter();

	// TODO: requested and enabled features.
};