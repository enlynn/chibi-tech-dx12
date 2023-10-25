#pragma once

#include <types.h>

#include "d3d12_common.h"

#include "gfx_resource.h"
#include "gfx_descriptor_allocator.h"

class platform_window;
class gfx_device;
class gfx_command_queue;

struct gfx_swapchain_info
{
	// Required to set.
	gfx_device*               mDevice                    = nullptr;
	gfx_command_queue*        mPresentQueue              = nullptr;
	cpu_descriptor_allocator* mRenderTargetDesciptorHeap = nullptr;
	// Optional settings
	u32                      mBackbufferCount      = 2;                          // TODO(enlynn): Determine the correct number of backbuffers
	DXGI_FORMAT              mSwapchainFormat      = DXGI_FORMAT_R8G8B8A8_UNORM; // TODO(enlynn): HDR?
	bool                     mAllowTearing         = false;                      // TODO(enlynn): How should tearing be supported?
	bool                     mVSyncEnabled         = true;
};

class gfx_swapchain
{
public:
	gfx_swapchain() = default;
	gfx_swapchain(gfx_swapchain_info& Info, const platform_window& ClientWindow);

	~gfx_swapchain() { /* if (mHandle) Deinit(); */ }
	void Deinit();

	u64 Present();

	const gfx_resource&   GetCurrentBackbuffer()   const { return mBackbuffers[mBackbufferIndex]; }
	const cpu_descriptor& GetCurrentRenderTarget() const { return mDescriptors[mBackbufferIndex]; }

	static constexpr u32 cMaxBackBufferCount = DXGI_MAX_SWAP_CHAIN_BUFFERS;

private:
	gfx_swapchain_info mInfo;

	IDXGISwapChain3* mHandle                                    = nullptr;
	u64              mBackbufferIndex                           = 0;
	gfx_resource     mBackbuffers[cMaxBackBufferCount]          = {};
	u64              mFenceValues[cMaxBackBufferCount]          = {};
	u32              mWidth                                     = 0;
	u32              mHeight                                    = 0;
	cpu_descriptor   mDescriptors[cMaxBackBufferCount]          = {};
};