#include "simple_renderer.h"
#include "dx12/gfx_device.h"
#include "dx12/gfx_command_queue.h"
#include "dx12/gfx_resource.h"
#include "dx12/gfx_descriptor_allocator.h"
#include "dx12/gfx_swapchain.h"

#include <types.h>
#include <util/allocator.h>
#include <platform/platform.h>

// TODO(enlynn): Remove this header from the renderer
#include "dx12/d3d12_common.h"

var_global allocator  gHeapAllocator = allocator::Default();
var_global u64        gFrameCount    = 0;

var_global gfx_device        gDevice        = {};
var_global gfx_command_queue gGraphicsQueue;
var_global gfx_swapchain     gSwapchain     = {};

// Descriptors
var_global cpu_descriptor_allocator   gCpuDescriptors[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {};


// Let's create a GPU 

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

#if 0

struct gfx_global
{
	allocator                  mHeapAllocator;

	gfx_device                 mDevice;
	gfx_swapchain              mSwapchain;

	gfx_command_queue          mGraphicsQueue;
	gfx_command_queue          mComputeQueue;
	gfx_command_queue          mCopyQueue;

	cpu_descriptor_allocator   mStaticDescriptors[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];  // Global descriptors, long lived
	//gpu_descriptor_allocator   mDynamicDescriptors[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES]; // Per Frame descriptors, lifetime is a single frame

	//gfx_resource_tracker       mResourceStates;                                           // Tracks global resource state

	//gfx_mesh_storage           mMeshStorage;
	//gfx_texture_storage        mTextureStorage;
	//gfx_material_storage       mMaterialStorage;
};

struct per_frame_cache
{
	gfx_global&             Global;

	//darray<resource*>       StaleResources;         // Resources that needs to be freed. Is freed on next use
	//darray<gfx_upload_data> ToUploadData;           // Resources that needs to be placed on the GPU. 

	//gfx_resource_tracker    ResourceStateCache;     // Tracks Temporary resource state. Often is flushed before a draw call
};

constexpr  int              cMaxFrameCache                 = 5; // Keep up to 5 frames
var_global per_frame_cache  gPerFrameCache[cMaxFrameCache];
var_global per_frame_cache& gFrameCache;

var_global gpu_descriptor_allocator gGpuDescriptors;


struct gfx_render_pass
{
	gpu_descriptor_allocation DynamicDescriptors[MAX_DESCRIPTOR_TYPES]; // Lifetime is a single frame.
};

struct forward_pass : public gfx_render_pass
{
	// Needs 3 SRV Descriptors
	// Needs 1 UAV Descriptor

	// GPU descriptor SRV Material Table

};

struct gfx_material_storage
{
	// gpu_descriptor { .ptr = nullptr }

	gpu_descriptor* mGpuDescriptors;   // Per-Frame, size based on the GpuCache
	material_data*  mMaterialStorage;  // All possible materials
	gpu_buffer*     mMaterialGpuCache; // Data on the GPU

	// and so forth
};

#endif


void
SimpleRendererInit(simple_renderer_info& RenderInfo)
{
	if (RenderInfo.HeapAllocator)
		gHeapAllocator = RenderInfo.HeapAllocator->Clone();

	gDevice.Init();
	gGraphicsQueue = gfx_command_queue(gHeapAllocator, gfx_command_queue_type::graphics, &gDevice);

	// Create the CPU Descriptor Allocators
	ForRange(u32, i, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES)
	{
		gCpuDescriptors[i].Init(&gDevice, gHeapAllocator, (D3D12_DESCRIPTOR_HEAP_TYPE)i);
	}

	gfx_swapchain_info SwapchainInfo = {
		.mDevice                    = &gDevice,
		.mPresentQueue              = &gGraphicsQueue,
		.mRenderTargetDesciptorHeap = &gCpuDescriptors[D3D12_DESCRIPTOR_HEAP_TYPE_RTV],
		// Leave everything else at their defaults for now...
	};

	gSwapchain = gfx_swapchain(SwapchainInfo, RenderInfo.ClientWindow);
}

void
SimpleRendererRender()
{
	//gFrameCache = gPerFrameCache[gFrameCount % cMaxFrameCache];

	gGraphicsQueue.ProcessCommandLists();

	const gfx_resource& Backbuffer            = gSwapchain.GetCurrentBackbuffer();
	const cpu_descriptor& SwapchainDescriptor = gSwapchain.GetCurrentRenderTarget();

	gfx_command_list* List = gGraphicsQueue.GetCommandList();

	D3D12_RESOURCE_BARRIER ToClearBarrier = {};
	ToClearBarrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	ToClearBarrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ToClearBarrier.Transition.pResource   = Backbuffer.AsHandle();
	ToClearBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
	ToClearBarrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
	ToClearBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	List->AsHandle()->ResourceBarrier(1, &ToClearBarrier);

	f32 ClearColor[4] = { 1.0f, 0.0f, 1.0f, 1.0f };
	List->AsHandle()->ClearRenderTargetView(SwapchainDescriptor.GetDescriptorHandle(), ClearColor, 0, nullptr);

	D3D12_RESOURCE_BARRIER ToRenderBarrier = {};
	ToRenderBarrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	ToRenderBarrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ToRenderBarrier.Transition.pResource   = Backbuffer.AsHandle();
	ToRenderBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	ToRenderBarrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT; //D3D12_RESOURCE_STATE_RENDER_TARGET
	ToRenderBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	List->AsHandle()->ResourceBarrier(1, &ToRenderBarrier);

	gGraphicsQueue.ExecuteCommandLists(farray(&List, 1));

	gSwapchain.Present();

	gFrameCount += 1;
}

void
SimpleRendererDeinit()
{
	gSwapchain.Deinit();

	ForRange(u32, i, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES)
	{
		gCpuDescriptors[i].Deinit();
	}

	gGraphicsQueue.Deinit();
	gDevice.Deinit();
}