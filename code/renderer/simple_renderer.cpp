#include "simple_renderer.h"
#include "dx12/gfx_device.h"
#include "dx12/gfx_command_queue.h"
#include "dx12/gfx_resource.h"

#include <types.h>
#include <util/allocator.h>
#include <platform/platform.h>

// TODO(enlynn): Remove this header from the renderer
#include "dx12/d3d12_common.h"

var_global allocator  gHeapAllocator = allocator::Default();

var_global gfx_device        gDevice        = {};
var_global gfx_command_queue gGraphicsQueue;

// Swapchain State
var_global constexpr u32              cMaxBackBufferCount = DXGI_MAX_SWAP_CHAIN_BUFFERS;
var_global constexpr u32              cBackbufferCount    = 2;                          // TODO(enlynn): Determine the correct number of backbuffers
var_global constexpr DXGI_FORMAT      cSwapchainFormat    = DXGI_FORMAT_R8G8B8A8_UNORM; // TODO(enlynn): HDR?
var_global constexpr bool             cAllowTearing       = false;                      // TODO(enlynn): How should tearing be supported?
var_global constexpr bool             cVSyncEnabled       = true;
var_global           IDXGISwapChain3* gSwapchain          = nullptr;
var_global           u64              gBackbufferIndex    = 0;
var_global           gfx_resource     gBackbuffers[cMaxBackBufferCount]{};
var_global           u64              gSwapchainFenceValues[cMaxBackBufferCount]{};     // Just declare the max possible buffers since different modes require different sizes
var_global           u32              gSwapchainWidth     = 0;
var_global           u32              gSwapchainHeight    = 0;

// 
// Thinking about descriptors:
// 
// Types of Descriptors:
// - Render Target Views
// - Depth Stencil Views
// - Shader Resource Views
// - Unordered Access Views
// - Constant Buffer Views
// - Index Buffer Views
// - Vertex Buffer Views
// - Stream Output Views (oo, what are these?)
// 
// Descirptors vary in size, so must query their size with ID3D12Device::GetDescriptorHandleIncrementSize
// - SRV, UAV, CBV need to do this
// 
// The App must manage descriptor information and make sure state is managed
// correctly.
// 
// The Driver does not store any data about descriptors. Their primary use is to be
// placed into descriptor heaps.
// 
// CPU Descriptor Handles 
// - Can be immediately used
// - Can be resused or their underlying heap disposed of
// 
// GPU Descsriptor Handles
// - Cannot be immedeiately used
// - Identifies locations on a command list or to be used on the GPU during execution
// 
// Null Descriptors
// - Often used to "fill" Descriptor slots in a Descriptor Array
// 
//
// Descriptor Heaps
// 
// From the docs:
// 
// A more efficient strategy than the basic one would be to pre-fill descriptor heaps 
// with descriptors required for the objects (or materials) that are known to be part of 
// the scene. The idea here is that it is only necessary to set the descriptor table at 
// draw time, as the descriptor heap is populated ahead of time.
// 
// More from the docs:
// 
// A variation of the pre-filling strategy is to treat the descriptor heap as one huge 
// array, containing all the required descriptors in fixed known locations. Then the 
// draw call needs only to receive a set of constants which are the indices into the 
// array of where the descriptors are that need to be used.
// 
// And finally:
// 
// A further optimization is to ensure root constants and root descriptors contain those 
// that change most frequently, rather than place constants in the descriptor heap. 
// For most hardware this is an efficient way of handling constants.
// 
// Shader Visible Descriptor Heap
// 
// - If allocating a descriptor heap smaller than the Device Limit may result in the 
//   Deiver sub-allocating from a larget heap to prevent swapping heaps. Otherwise
//   the Driver may force a Wait Idle
// 
// - The recommendation to avoid the GPU Idle was kind of vague ... the docs suggested
//   to "take advantage" of the idle by forcing the wait at certain intervals?
// 
// Non Shader Visible Descriptor Heap
// - All Descriptor Heaps can be minupalted on the CPU
// - Cannot be used by Shader Visible Heaps
// - RTVs and DSVs are always Non-Shader visible 
// - IBVs, VBVs, and SOVs do not have specifc heap types and are passed directly to API methods
// - After recording into the Command List (i.e. OMSetRenderTargets) the descriptors are
//   immediately ready for re-use
// - Implemnentations can choose to bake Descriptor Tables into the command list at recording
//   to avoid dereferncing the table pointer at execution
// 
// Descriptor Visiblity Summary:
//                | Shader Vis, CPU Write only | Non-Shader Vis, CPU Read/Write
// CVB, SRV, UAV  | Yes                        | Yes
// Sampler        | Yes                        | Yes
// RTV            | No                         | Yes
// DSV            | No                         | Yes
// 
// 
// 
// How I previously handled descriptors:
// 
// Types:
// - (GPU/Shader Visible) Descriptor Allocator
// - (CPU Visible) Dynamic Descriptor Allocator
// 
// Variables:
// - GlobalDescriptorAllocators (managed by the device)
// - Dynamic Descriptor Allocators (managed by the command list)
// --- Allows for descriptors to be staged before being committed to the 
//     command list list. Dyn Desc need to be committed before a draw/dispatch
// - Bound Descriptor Heaps (managed by the command list)
// --- Reduce the number of heap binds per frame
// 
// I have to unwind this web of dependencies ... originally all per frame resources
// were tied to a Command List
// - Dynamic Descriptors
// - GC Resources
// - ResourceStateTracking (local state tracker - calls into a global tracker)
// 
// Global State that is called everywhere
// - Resource State Tracking
// - GPU Descriptor Allocator
// 
// Some things to think about
// - When do we need to "write" into the Global Resource Tracker?
// --- When does the command list does this?
// - When do we create GPU Resources?
// - When does the Command List write dynamic descriptors?
// - Do Dynamic Descriptors write to the GPU Descriptors?
// 
// 


fn_internal void 
PresentSwapchain()
{
}

fn_internal void
CreateSwapchain(const platform_window& ClientWindow)
{
	const platform_window_rect   WindowRect   = ClientWindow.GetWindowRect();
	const platform_window_handle WindowHandle = ClientWindow.GetHandle();

	auto DeviceHandle  = gDevice.AsHandle();
	auto DeviceAdapter = gDevice.AsAdapter();

	IDXGIFactory5* DeviceFactory = nullptr;
	AssertHr(DeviceAdapter->GetParent(ComCast(&DeviceFactory)));

	// Is Tearing supported?
	bool IsTearingSupported = cAllowTearing;
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
	SwapchainDesc.Format      = cSwapchainFormat;
	SwapchainDesc.Stereo      = FALSE;
	SwapchainDesc.SampleDesc  = { .Count = 1, .Quality = 0 }; // Number of multisamples per pixels
	SwapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapchainDesc.BufferCount = cBackbufferCount;
	SwapchainDesc.Scaling     = DXGI_SCALING_STRETCH;
	SwapchainDesc.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	SwapchainDesc.AlphaMode   = DXGI_ALPHA_MODE_UNSPECIFIED;
	SwapchainDesc.Flags       = Flags;

	IDXGISwapChain1* SwapchainBase = nullptr;
	AssertHr(DeviceFactory->CreateSwapChainForHwnd(gGraphicsQueue.AsHandle(), (HWND)WindowHandle, &SwapchainDesc, nullptr, nullptr, &SwapchainBase));

	// Disable the automatic Alt+Enter fullscreen toggle. We'll do this ourselves to support adaptive refresh rate stuff.
	AssertHr(DeviceFactory->MakeWindowAssociation((HWND)WindowHandle, DXGI_MWA_NO_ALT_ENTER));

	// Let's get the actual swapchain
	AssertHr(SwapchainBase->QueryInterface(ComCast(&gSwapchain)));
	SwapchainBase->Release();

	gBackbufferIndex = gSwapchain->GetCurrentBackBufferIndex();
	
	gSwapchain->SetMaximumFrameLatency(cBackbufferCount - 1);

	gSwapchainWidth  = WindowRect.Width;
	gSwapchainHeight = WindowRect.Height;

	ForRange(u32, i, cMaxBackBufferCount)
	{
		gSwapchainFenceValues[i] = 0;
	}

	// TODO: update render target views
	// TODO: Wouldn't work for resizing. Technically, we are not guarenteed for the number of backbuffers to
	// remain the same. Will need to iterate over this list and free the max and then initialize new buffers.
	ForRange(u32, i, cBackbufferCount)
	{
		ID3D12Resource* Backbuffer = nullptr;
		AssertHr(gSwapchain->GetBuffer(i, ComCast(&Backbuffer)));

		D3D12_CLEAR_VALUE ClearValue = {};
		ClearValue.Format   = cSwapchainFormat;
		ClearValue.Color[0] = 0.0f;
		ClearValue.Color[1] = 0.0f;
		ClearValue.Color[2] = 0.0f;
		ClearValue.Color[3] = 1.0f;

		gBackbuffers[i].Release(); // Release any lingering buffers
		gBackbuffers[i] = gfx_resource(gDevice, Backbuffer, ClearValue);
	}

	ComSafeRelease(DeviceFactory);
}

fn_internal u64
SwapchainPresent()
{
	const gfx_resource& CurrentBackbuffer = gBackbuffers[gBackbufferIndex];
	const UINT          SyncInterval      = cVSyncEnabled ? 1 : 0;

	u32 PresentFlags = 0;
	if (SyncInterval == 0)
	{
		DXGI_SWAP_CHAIN_DESC1 Desc = {};
		AssertHr(gSwapchain->GetDesc1(&Desc));

		if ((Desc.Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING) != 0)
			PresentFlags |= DXGI_PRESENT_ALLOW_TEARING;
	}

	AssertHr(gSwapchain->Present(SyncInterval, PresentFlags));

	gSwapchainFenceValues[gBackbufferIndex] = gGraphicsQueue.Signal();
	gBackbufferIndex = gSwapchain->GetCurrentBackBufferIndex();
	
	gGraphicsQueue.WaitForFence(gSwapchainFenceValues[gBackbufferIndex]);

	return gBackbufferIndex;
}

fn_internal void
SwapchainRelease()
{
	ForRange(u32, i, cMaxBackBufferCount)
	{ // Release all possible backbuffers
		gBackbuffers[i].Release();
	}

	ComSafeRelease(gSwapchain);
}

void
SimpleRendererInit(simple_renderer_info& RenderInfo)
{
	if (RenderInfo.HeapAllocator)
		gHeapAllocator = RenderInfo.HeapAllocator->Clone();

	gDevice.Init();
	gGraphicsQueue = gfx_command_queue(gHeapAllocator, gfx_command_queue_type::graphics, &gDevice);

	CreateSwapchain(RenderInfo.ClientWindow);
}

void
SimpleRendererRender()
{
	SwapchainPresent();

	gGraphicsQueue.ProcessCommandLists();
}

void
SimpleRendererDeinit()
{
	SwapchainRelease();
	gGraphicsQueue.Deinit();
	gDevice.Deinit();
}