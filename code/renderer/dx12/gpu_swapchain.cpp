#include "gpu_swapchain.h"
#include "gpu_device.h"
#include "gpu_command_queue.h"
#include "gpu_state.h"

#include <platform/platform.h>

gpu_swapchain::gpu_swapchain(gpu_frame_cache* FrameCache, gpu_swapchain_info& Info, const platform_window& ClientWindow)
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

	ForRange(u32, i, cMaxBackBufferCount)
	{
		mFenceValues[i] = 0;
	}

    // It is my expectation the number of Buffers we requested, matches the number actually created.
    DXGI_SWAP_CHAIN_DESC1 Desc1 = {};
    mHandle->GetDesc1(&Desc1);
    assert(Desc1.BufferCount == Info.mBackbufferCount);

    // Make sure the width/height is accurate to the swapchain and not the client window
    mWidth  = Desc1.Width;
    mHeight = Desc1.Height;

	// Update render target views
    UpdateRenderTargetViews(FrameCache);

	ComSafeRelease(DeviceFactory);
}
void gpu_swapchain::UpdateRenderTargetViews(gpu_frame_cache* FrameCache)
{
    // TODO: Wouldn't work for resizing. Technically, we are not guaranteed for the number of backbuffers to
    // remain the same. Will need to iterate over this list and free the max and then initialize new buffers.
    //
    // So for now, it seems we will keep the same number of backbuffers. Now, this isn't always advisable,
    // but will suffice at the moment.
    //
    ForRange(u32, i, mInfo.mBackbufferCount)
    {
        ID3D12Resource* D3DBackbuffer = nullptr;
        AssertHr(mHandle->GetBuffer(i, ComCast(&D3DBackbuffer)));

        D3D12_CLEAR_VALUE ClearValue = {};
        ClearValue.Format = mInfo.mSwapchainFormat;
        ClearValue.Color[0] = 0.0f;
        ClearValue.Color[1] = 0.0f;
        ClearValue.Color[2] = 0.0f;
        ClearValue.Color[3] = 1.0f;

        // When resizing the swpachain, we can have any lingering backbuffer images. Have to immediate release
        // the underlying resource.
        if (mBackbuffers[i].GetResource()->AsHandle())
        {
            const gpu_resource* Resource = mBackbuffers[i].GetResource();
            FrameCache->RemoveTrackedResource(*Resource);
            mBackbuffers[i].ReleaseUnsafe(FrameCache);
        }

        gpu_resource Backbuffer = gpu_resource(*mInfo.mDevice, D3DBackbuffer, ClearValue);
        FrameCache->TrackResource(Backbuffer, D3D12_RESOURCE_STATE_COMMON);

        mBackbuffers[i] = gpu_texture(FrameCache, Backbuffer);

        // Create the Render Target View
        // NOTE(enlynn): the texture should handle this
        //mInfo.mRenderTargetDesciptorHeap->ReleaseDescriptors(mDescriptors[i]);
        //mDescriptors[i] = mInfo.mRenderTargetDesciptorHeap->Allocate();
        //assert(!mDescriptors[i].IsNull());
        //mInfo.mDevice->AsHandle()->CreateRenderTargetView(mBackbuffers[i].AsHandle(), nullptr, mDescriptors[i].GetDescriptorHandle());
    }
}

void gpu_swapchain::Resize(gpu_frame_cache* FrameCache, u32 Width, u32 Height)
{
    if (mWidth != Width || mHeight != Height)
    {
        // Cannot create a window smaller than (1, 1)
        mWidth  = FastMax(1, Width);
        mHeight = FastMax(1, Height);

        // Make sure no more work is being executed on any work queue
        // TODO(enlynn): this should probably be the responsibility of the renderer, not the swapchain.
        FrameCache->FlushGPU();

        DXGI_SWAP_CHAIN_DESC1 Desc1 = {};
        mHandle->GetDesc1(&Desc1);

        // Always want the waitable object flag, and the tearing flag when supported
        u32 Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        if ((Desc1.Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING) != 0)
            Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

        AssertHr(mHandle->ResizeBuffers(mInfo.mBackbufferCount, 0, 0, DXGI_FORMAT_UNKNOWN, Flags));

        mBackbufferIndex = mHandle->GetCurrentBackBufferIndex();
        UpdateRenderTargetViews(FrameCache);
    }
}


void 
gpu_swapchain::Release(gpu_frame_cache* FrameCache)
{
	ForRange(u32, i, cMaxBackBufferCount)
	{ // Release all possible backbuffers
		mBackbuffers[i].ReleaseUnsafe(FrameCache);
	}

	ComSafeRelease(mHandle);

	mInfo = {};
}

gpu_render_target*
gpu_swapchain::GetRenderTarget()
{
    mRenderTarget.Reset();
    mRenderTarget.AttachTexture(attachment_point::color0, &mBackbuffers[mBackbufferIndex]);
    // TODO(enlynn): check if there is a depth buffer present.
    return &mRenderTarget;
}

u64 
gpu_swapchain::Present()
{
	const UINT SyncInterval = mInfo.mVSyncEnabled ? 1 : 0;

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
	mBackbufferIndex               = mHandle->GetCurrentBackBufferIndex();

	mInfo.mPresentQueue->WaitForFence(mFenceValues[mBackbufferIndex]);

	return mBackbufferIndex;
}