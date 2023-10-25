#include "gfx_swapchain.h"
#include "gfx_device.h"
#include "gfx_command_queue.h"

#include <platform/platform.h>

gfx_swapchain::gfx_swapchain(gfx_swapchain_info& Info, const platform_window& ClientWindow)
	: mInfo(Info)
{
	assert(mInfo.mDevice);
	assert(mInfo.mPresentQueue);
	assert(mInfo.mRenderTargetDesciptorHeap);

	const platform_window_rect   WindowRect   = ClientWindow.GetWindowRect();
	const platform_window_handle WindowHandle = ClientWindow.GetHandle();

	auto DeviceHandle  = mInfo.mDevice->AsHandle();
	auto DeviceAdapter = mInfo.mDevice->AsAdapter();

	IDXGIFactory5* DeviceFactory = nullptr;
	AssertHr(DeviceAdapter->GetParent(ComCast(&DeviceFactory)));

	// Is Tearing supported?
	bool IsTearingSupported = mInfo.mAllowTearing;
	if (IsTearingSupported)
	{
		if (FAILED(DeviceFactory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &IsTearingSupported, sizeof(IsTearingSupported))))
		{
			IsTearingSupported = false;
		}

		if (!IsTearingSupported)
		{
			LogWarn("Tearing support requested, but not available.");
		}
	}

	u32 Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
	if (IsTearingSupported) Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	DXGI_SWAP_CHAIN_DESC1 SwapchainDesc = {};
	SwapchainDesc.Width       = WindowRect.Width;
	SwapchainDesc.Height      = WindowRect.Height;
	SwapchainDesc.Format      = mInfo.mSwapchainFormat;
	SwapchainDesc.Stereo      = FALSE;
	SwapchainDesc.SampleDesc  = { .Count = 1, .Quality = 0 }; // Number of multisamples per pixels
	SwapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapchainDesc.BufferCount = mInfo.mBackbufferCount;
	SwapchainDesc.Scaling     = DXGI_SCALING_STRETCH;
	SwapchainDesc.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	SwapchainDesc.AlphaMode   = DXGI_ALPHA_MODE_UNSPECIFIED;
	SwapchainDesc.Flags       = Flags;

	IDXGISwapChain1* SwapchainBase = nullptr;
	AssertHr(DeviceFactory->CreateSwapChainForHwnd(mInfo.mPresentQueue->AsHandle(), (HWND)WindowHandle, &SwapchainDesc, nullptr, nullptr, &SwapchainBase));

	// Disable the automatic Alt+Enter fullscreen toggle. We'll do this ourselves to support adaptive refresh rate stuff.
	AssertHr(DeviceFactory->MakeWindowAssociation((HWND)WindowHandle, DXGI_MWA_NO_ALT_ENTER));

	// Let's get the actual swapchain
	AssertHr(SwapchainBase->QueryInterface(ComCast(&mHandle)));
	SwapchainBase->Release();

	mBackbufferIndex = mHandle->GetCurrentBackBufferIndex();

	mHandle->SetMaximumFrameLatency(mInfo.mBackbufferCount - 1);

	mWidth  = WindowRect.Width;
	mHeight = WindowRect.Height;

	ForRange(u32, i, cMaxBackBufferCount)
	{
		mFenceValues[i] = 0;
	}

	// TODO: update render target views
	// TODO: Wouldn't work for resizing. Technically, we are not guarenteed for the number of backbuffers to
	// remain the same. Will need to iterate over this list and free the max and then initialize new buffers.
	ForRange(u32, i, mInfo.mBackbufferCount)
	{
		ID3D12Resource* Backbuffer = nullptr;
		AssertHr(mHandle->GetBuffer(i, ComCast(&Backbuffer)));

		D3D12_CLEAR_VALUE ClearValue = {};
		ClearValue.Format = mInfo.mSwapchainFormat;
		ClearValue.Color[0] = 0.0f;
		ClearValue.Color[1] = 0.0f;
		ClearValue.Color[2] = 0.0f;
		ClearValue.Color[3] = 1.0f;

		mBackbuffers[i].Release(); // Release any lingering buffers
		mBackbuffers[i] = gfx_resource(*mInfo.mDevice, Backbuffer, ClearValue);

		// Create the Render Target View
		mInfo.mRenderTargetDesciptorHeap->ReleaseDescriptors(mDescriptors[i]);

		mDescriptors[i] = mInfo.mRenderTargetDesciptorHeap->Allocate();
		assert(!mDescriptors[i].IsNull());

		mInfo.mDevice->AsHandle()->CreateRenderTargetView(mBackbuffers[i].AsHandle(), nullptr, mDescriptors[i].GetDescriptorHandle());
	}

	ComSafeRelease(DeviceFactory);
}

void 
gfx_swapchain::Deinit()
{
	ForRange(u32, i, cMaxBackBufferCount)
	{ // Release all possible backbuffers
		mInfo.mRenderTargetDesciptorHeap->ReleaseDescriptors(mDescriptors[i]);
		mBackbuffers[i].Release();
	}

	ComSafeRelease(mHandle);

	mInfo = {};
}

u64 
gfx_swapchain::Present()
{
	const gfx_resource& CurrentBackbuffer = mBackbuffers[mBackbufferIndex];
	const UINT          SyncInterval      = mInfo.mVSyncEnabled ? 1 : 0;

	u32 PresentFlags = 0;
	if (SyncInterval == 0)
	{
		DXGI_SWAP_CHAIN_DESC1 Desc = {};
		AssertHr(mHandle->GetDesc1(&Desc));

		if ((Desc.Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING) != 0)
			PresentFlags |= DXGI_PRESENT_ALLOW_TEARING;
	}

	AssertHr(mHandle->Present(SyncInterval, PresentFlags));

	mFenceValues[mBackbufferIndex] = mInfo.mPresentQueue->Signal();
	mBackbufferIndex = mHandle->GetCurrentBackBufferIndex();

	mInfo.mPresentQueue->WaitForFence(mFenceValues[mBackbufferIndex]);

	return mBackbufferIndex;
}