#include "simple_renderer.h"
#include "d3d12/d3d12.h"
#include "dx12/gfx_device.h"
#include "dx12/gfx_command_queue.h"
#include "dx12/gfx_resource.h"
#include "dx12/gfx_descriptor_allocator.h"
#include "dx12/gfx_swapchain.h"
#include "dx12/gfx_utils.h"
#include "dx12/gfx_shader_utils.h"
#include "dx12/gfx_root_signature.h"
#include "dx12/gfx_pso.h"

#include <types.h>
#include <util/allocator.h>
#include <platform/platform.h>

// TODO(enlynn): Remove this header from the renderer
#include "dx12/d3d12_common.h"
#include "renderer/dx12/gfx_resource.h"

enum class gfx_buffer_type : u8
{
	byte_address,
	structured,
	constant,
	index,
	vertex,
	max,
};

enum class triangle_root_parameter
{
	vertex_buffer = 0,
	per_draw      = 1,
};

struct gfx_buffer
{
	gfx_buffer_type              mType;
	gfx_resource                 mResource;

	union
	{ // Buffer Views for Vertex and Index Buffer
		D3D12_INDEX_BUFFER_VIEW  mIndexView;
		D3D12_VERTEX_BUFFER_VIEW mVertexView;
	};
	
	// Don't store the SRV/CBV/UAVs here. For most other buffer types, we store a large arrays 
	// and just bind the array rather than individual buffer views. There are a few exceptions
	// to this rule - Index Buffers, Vertex Buffers, and Byte Address -but-acutally-Vertex Views, 
	
	// Structured and Byte Address Buffers are bound as SRVs ?
	
	// Some extra data for IndexBuffers
	u8  mStride;
	u64 mIndexCount;
};

struct gfx_global
{
	allocator                  mHeapAllocator = {};
	u64                        mFrameCount    = 0;

	gfx_device                 mDevice        = {};
	gfx_swapchain              mSwapchain     = {};

	gfx_command_queue          mGraphicsQueue = {};
	gfx_command_queue          mComputeQueue;
	gfx_command_queue          mCopyQueue;

	cpu_descriptor_allocator   mStaticDescriptors[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];  // Global descriptors, long lived
	//gpu_descriptor_allocator   mDynamicDescriptors[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES]; // Per Frame descriptors, lifetime is a single frame

	//gfx_resource_tracker       mResourceStates;                                           // Tracks global resource state

	//gfx_mesh_storage           mMeshStorage;
	//gfx_texture_storage        mTextureStorage;
	//gfx_material_storage       mMaterialStorage;

	// Temporary
	gfx_buffer         mVertexResource = {};
	cpu_descriptor     mVertexSrv      = {}; // Ideally we wouldn't have a SRV for individual buffers?

	gfx_buffer         mIndexResource  = {};

	gfx_root_signature mSimpleSignature = {};
	gfx_pso            mSimplePipeline  = {};
};

struct per_frame_cache
{
	gfx_global*             Global         = nullptr;
	darray<gfx_resource>    mStaleResources = {};         // Resources that needs to be freed. Is freed on next use
	//darray<gfx_upload_data> ToUploadData;              // Resources that needs to be placed on the GPU. 

	//gfx_resource_tracker    ResourceStateCache;        // Tracks Temporary resource state. Often is flushed before a draw call

	ID3D12DescriptorHeap*   mBoundDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
};

struct vertex_draw_constants
{
	u32 uVertexOffset      = 0;
	u32 uVertexBufferIndex = 0;
};

constexpr  int              cMaxFrameCache                 = 5; // Keep up to 5 frames
var_global per_frame_cache  gPerFrameCache[cMaxFrameCache] = {}; 
var_global per_frame_cache* gFrameCache                    = nullptr; 
var_global gfx_global       gGlobal                        = {};

//var_global gpu_descriptor_allocator gGpuDescriptors;

// TODO: Refactor out the "CopyBuffer" portion so we can reuse it across Buffer types.

fn_internal gfx_resource
CopyBuffer(gfx_command_list* CopyList, void* BufferData, u64 BufferSize, D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATES InitialBufferState = D3D12_RESOURCE_STATE_COMMON)
{
	const gfx_device& Device = gGlobal.mDevice;

	gfx_resource Result = {};
	if (BufferSize == 0)
	{
		// This will result in a NULL resource (which may be desired to define a default null resource).
	}
	else
	{
		D3D12_HEAP_PROPERTIES HeapProperties = GetHeapProperties();
		D3D12_RESOURCE_DESC BufferDesc = GetBufferResourceDesc(
			BufferSize, Flags);
        
		ID3D12Resource* TempResource = nullptr;
		AssertHr(Device.AsHandle()->CreateCommittedResource(
			&HeapProperties, 
			D3D12_HEAP_FLAG_NONE,
			&BufferDesc, 
			D3D12_RESOURCE_STATE_COMMON, 
			nullptr,
			ComCast(&TempResource)
		));

		Result = gfx_resource(Device, TempResource);
        
		// TODO: Track resource state.

		// Add the resource to the global resource state tracker.
		//ResourceStateTracker::AddGlobalResourceState(result, D3D12_RESOURCE_STATE_COMMON);
        
		if (BufferData != nullptr )
		{
			// Create an intermediary buffer. Note, this should only be necessary
			// for GPU-only buffers.
			D3D12_HEAP_PROPERTIES InterHeapProperties = GetHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
			D3D12_RESOURCE_DESC   InterBufferDesc     = GetBufferResourceDesc(BufferSize);

			// Create an upload resource to use as an intermediate buffer to copy the buffer resource
			ID3D12Resource* UploadResourceTemp;
			AssertHr(Device.AsHandle()->CreateCommittedResource(
				&InterHeapProperties, 
				D3D12_HEAP_FLAG_NONE,
				&InterBufferDesc, 
				D3D12_RESOURCE_STATE_GENERIC_READ, 
				nullptr,
				ComCast(&UploadResourceTemp)
			));

			gfx_resource UploadResource = gfx_resource(Device, UploadResourceTemp);
            
			D3D12_SUBRESOURCE_DATA SubresourceData = {};
			SubresourceData.pData                  = BufferData;
			SubresourceData.RowPitch               = (LONG_PTR)BufferSize;
			SubresourceData.SlicePitch             = SubresourceData.RowPitch;

			{
				gfx_transition_barrier BufferCopyDstBarrier = {};
				BufferCopyDstBarrier.BeforeState = D3D12_RESOURCE_STATE_COMMON;
				BufferCopyDstBarrier.AfterState  = D3D12_RESOURCE_STATE_COPY_DEST;
				CopyList->TransitionBarrier(Result, BufferCopyDstBarrier);
			}
            
			CopyList->UpdateSubresources<1>(
				Result, UploadResource, 0, 
				0, 1, &SubresourceData
			);
            
			// Add references to resources so they stay in scope until the command list is reset.
			gFrameCache->mStaleResources.PushBack(UploadResource);
		}
        
		//TrackResource(result);
	}

	{ // Transition the buffer back to be used
		gfx_transition_barrier BufferBarrier = {};
		BufferBarrier.BeforeState = D3D12_RESOURCE_STATE_COPY_DEST;
		BufferBarrier.AfterState  = InitialBufferState;
		CopyList->TransitionBarrier(Result, BufferBarrier);
	}

	return Result;
}

fn_internal gfx_buffer 
CreateByteAddressBuffer(gfx_command_list* List, void* BufferData, u64 Count, u64 Stride = 1)
{
	u64 BufferSize = Count * Stride;
	// Not sure if this should be D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER or D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
	gfx_resource BufferResource = CopyBuffer(List, BufferData, BufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	gfx_buffer Result = {};
	Result.mType      = gfx_buffer_type::byte_address;
	Result.mResource  = BufferResource;
	// Shader Resource View?

	return Result;
}

fn_internal gfx_buffer 
CreateIndexBuffer(gfx_command_list* List, void* BufferData, u64 Count, u64 Stride = sizeof(u32))
{
	u64 BufferSize = Count * Stride;
	gfx_resource BufferResource = CopyBuffer(List, BufferData, BufferSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_INDEX_BUFFER);

	gfx_buffer Result = {};
	Result.mType      = gfx_buffer_type::index;
	Result.mResource  = BufferResource;

	Result.mIndexView.BufferLocation = BufferResource.AsHandle()->GetGPUVirtualAddress();
	Result.mIndexView.Format         = (Stride == sizeof(u16)) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
	Result.mIndexView.SizeInBytes    = (UINT)(Count * Stride);

	Result.mStride     = (u8)Stride;
	Result.mIndexCount = Count;

	return Result;
}

void
SimpleRendererInit(simple_renderer_info& RenderInfo)
{
	if (RenderInfo.HeapAllocator)
		gGlobal.mHeapAllocator = RenderInfo.HeapAllocator->Clone();

	gGlobal.mDevice.Init();
	gGlobal.mGraphicsQueue = gfx_command_queue(gGlobal.mHeapAllocator, gfx_command_queue_type::graphics, &gGlobal.mDevice);

	// Create the CPU Descriptor Allocators
	ForRange(u32, i, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES)
	{
		gGlobal.mStaticDescriptors[i].Init(&gGlobal.mDevice, gGlobal.mHeapAllocator, (D3D12_DESCRIPTOR_HEAP_TYPE)i);
	}

	gfx_swapchain_info SwapchainInfo = {
		.mDevice                    = &gGlobal.mDevice,
		.mPresentQueue              = &gGlobal.mGraphicsQueue,
		.mRenderTargetDesciptorHeap = &gGlobal.mStaticDescriptors[D3D12_DESCRIPTOR_HEAP_TYPE_RTV],
		// Leave everything else at their defaults for now...
	};

	gGlobal.mSwapchain = gfx_swapchain(SwapchainInfo, RenderInfo.ClientWindow);

	//
	// Let's setup the the Per Frame Resources and let Frame 0 be the setup frame
	// 
	ForRange(int, i, cMaxFrameCache)
	{
		gPerFrameCache[i].Global          = &gGlobal;
		gPerFrameCache[i].mStaleResources = darray<gfx_resource>(gGlobal.mHeapAllocator, 5);
	}

	gFrameCache = &gPerFrameCache[0];

	//
	// Test Geometry
	//
		
	// Let's create a test resource
	struct vertex { f32 Pos[2]; f32 Col[3]; };
	vertex Vertices[] = {
		{ .Pos = { -0.5f, -0.5f }, .Col = { 1.0f, 0.0f, 0.0f } },
		{ .Pos = {  0.5f, -0.5f }, .Col = { 0.0f, 1.0f, 0.0f } },
		{ .Pos = {  0.0f,  0.5f }, .Col = { 0.0f, 0.0f, 1.0f } },
	};

	u16 Indices[] = { 0, 1, 2 };

	gfx_command_list* UploadList = gGlobal.mGraphicsQueue.GetCommandList();

	gGlobal.mVertexResource = CreateByteAddressBuffer(UploadList, (void*)Vertices, 
		ArrayCount(Vertices), sizeof(vertex));

	{ // Create the SRV for the Byte Address Buffer - NOTE: Not needed actually!
		D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
		SrvDesc.ViewDimension           = D3D12_SRV_DIMENSION_BUFFER;
		SrvDesc.Format                  = DXGI_FORMAT_R32_TYPELESS;
		SrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SrvDesc.Buffer.NumElements      = (UINT)sizeof(Vertices) / 4;
		SrvDesc.Buffer.Flags            = D3D12_BUFFER_SRV_FLAG_RAW;

		gGlobal.mVertexSrv = gGlobal.mStaticDescriptors[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].Allocate(1);
		gGlobal.mDevice.AsHandle()->CreateShaderResourceView(gGlobal.mVertexResource.mResource.AsHandle(), &SrvDesc, gGlobal.mVertexSrv.GetDescriptorHandle());
	}

	gGlobal.mIndexResource = CreateIndexBuffer(UploadList, Indices, 3, sizeof(Indices[0]));
	gGlobal.mGraphicsQueue.ExecuteCommandLists(&UploadList);
	gGlobal.mGraphicsQueue.Flush(); // Forcibly upload all of the geometry (for now)

	//
	// Register the builtin-shader resource and load 
	//

	resource_loader ShaderLoader = shader_resource::GetResourceLoader();
	RenderInfo.ResourceSystem.RegisterLoader(ShaderLoader);

	shader_resource VertexShader = shader_resource(shader_stage::vertex);
	shader_resource PixelShader  = shader_resource(shader_stage::pixel);

	RenderInfo.ResourceSystem.Load(resource_type::builtin_shader, "TestTriangle", &VertexShader);
	RenderInfo.ResourceSystem.Load(resource_type::builtin_shader, "TestTriangle", &PixelShader);

	{ // Create a simple root signature
		gfx_root_descriptor VertexBuffers[]    = { { .mRootIndex = u32(triangle_root_parameter::vertex_buffer), .mType = gfx_descriptor_type::srv } };
		gfx_root_constant   PerDrawConstants[] = { { .mRootIndex = u32(triangle_root_parameter::per_draw), .mNum32bitValues = sizeof(vertex_draw_constants) / 4 } };
		const char*         DebugName          = "Simple Root Signature";

		gfx_root_signature_info Info = {};
		Info.Descriptors         = farray(VertexBuffers, ArrayCount(VertexBuffers));
		Info.DescriptorConstants = farray(PerDrawConstants, ArrayCount(PerDrawConstants));
		Info.Name                = DebugName;
		gGlobal.mSimpleSignature = gfx_root_signature(gGlobal.mDevice, Info);
	}

	{ // Create a simple pso
		gfx_pso_info Info = { .mRootSignature = gGlobal.mSimpleSignature };
		Info.mVertexShader = VertexShader.GetShaderBytecode();
		Info.mPixelShader  = PixelShader.GetShaderBytecode();

		D3D12_RT_FORMAT_ARRAY PsoRTFormats = {};
		PsoRTFormats.NumRenderTargets = 1;
		PsoRTFormats.RTFormats[0]     = gGlobal.mSwapchain.GetSwapchainFormat();
		Info.mRenderTargetFormats     = PsoRTFormats;

		gGlobal.mSimplePipeline = gfx_pso(gGlobal.mDevice, Info);
	}

	gGlobal.mFrameCount += 1;
}

fn_internal void
ReleaseStateResources(darray<gfx_resource>& StaleResources)
{
	for (gfx_resource& Resource : StaleResources)
	{
		ID3D12Resource* ResourceHandle = Resource.AsHandle();
		ComSafeRelease(ResourceHandle);
	}
	StaleResources.Clear();
}

void
SimpleRendererRender()
{
	gFrameCache = &gPerFrameCache[gGlobal.mFrameCount % cMaxFrameCache];
	
	// Update any pending command lists
	gGlobal.mGraphicsQueue.ProcessCommandLists();

	// Clean up any previous resources
	ReleaseStateResources(gFrameCache->mStaleResources);

	// Let's render!
	const gfx_resource&   Backbuffer          = gGlobal.mSwapchain.GetCurrentBackbuffer();
	const cpu_descriptor& SwapchainDescriptor = gGlobal.mSwapchain.GetCurrentRenderTarget();

	gfx_command_list* List = gGlobal.mGraphicsQueue.GetCommandList();

	gfx_transition_barrier ToClearBarrier = {};
	ToClearBarrier.BeforeState = D3D12_RESOURCE_STATE_COMMON;
	ToClearBarrier.AfterState  = D3D12_RESOURCE_STATE_RENDER_TARGET;
	List->TransitionBarrier(Backbuffer, ToClearBarrier);

	f32 ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	List->AsHandle()->ClearRenderTargetView(SwapchainDescriptor.GetDescriptorHandle(), ClearColor, 0, nullptr);

	// Set the render target (hardcode for now) 
	D3D12_CPU_DESCRIPTOR_HANDLE RtHandles[] = { SwapchainDescriptor.GetDescriptorHandle() };
	List->AsHandle()->OMSetRenderTargets(1, RtHandles, FALSE, nullptr);

	// Scissor / Viewport
	D3D12_RESOURCE_DESC BackbufferDesc = Backbuffer.GetResourceDesc();

	D3D12_VIEWPORT Viewport = {
		0.0f,                        // TopLeftX, Width  * Bias.x  (Bias.x == 0.0 by default)
		0.0f,                        // TopLeftY, Height * Bias.y  (Bias.y == 0.0 by default)
		f32(BackbufferDesc.Width),   // Width,    Width  * Scale.x (Scale.x == 1.0 by default)
		f32(BackbufferDesc.Height),  // Height,   Height * Scale.y (Scale.y == 1.0 by default)
		0.0f,                        // MinDepth
		1.0f                         // MaxDepth
	};
	List->SetViewport(Viewport);

	D3D12_RECT ScissorRect = {};
	ScissorRect.left   = 0;
	ScissorRect.top    = 0;
	ScissorRect.right  = (LONG)Viewport.Width;
	ScissorRect.bottom = (LONG)Viewport.Height;
	List->SetScissorRect(ScissorRect);

	// Draw some geomtry

	List->SetPipelineState(gGlobal.mSimplePipeline);
	List->SetGraphicsRootSignature(gGlobal.mSimpleSignature);

	List->SetTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	List->SetIndexBuffer(gGlobal.mIndexResource.mIndexView);
	List->SetShaderResourceViewInline(u32(triangle_root_parameter::vertex_buffer), gGlobal.mVertexResource.mResource.AsHandle());

	vertex_draw_constants Constants = {};
	List->SetGraphics32BitConstants(u32(triangle_root_parameter::per_draw), &Constants);

	List->DrawIndexedInstanced((u32)gGlobal.mIndexResource.mIndexCount);

	gfx_transition_barrier ToRenderBarrier = {};
	ToRenderBarrier.BeforeState = D3D12_RESOURCE_STATE_RENDER_TARGET;
	ToRenderBarrier.AfterState  = D3D12_RESOURCE_STATE_PRESENT;
	List->TransitionBarrier(Backbuffer, ToRenderBarrier);

	gGlobal.mGraphicsQueue.ExecuteCommandLists(&List);

	gGlobal.mSwapchain.Present();

	gGlobal.mFrameCount += 1;
}

void
SimpleRendererDeinit()
{
	{ // Free the temp resources
		ID3D12Resource* ResourceHandle = gGlobal.mVertexResource.mResource.AsHandle();
		ComSafeRelease(ResourceHandle);

		ResourceHandle = gGlobal.mIndexResource.mResource.AsHandle();
		ComSafeRelease(ResourceHandle);
	}

	gGlobal.mSimplePipeline.Release();
	gGlobal.mSimpleSignature.Release();

	// Free any stale resources
	ForRange(int, i, cMaxFrameCache)
	{
		ReleaseStateResources(gPerFrameCache[i].mStaleResources);
	}

	gGlobal.mSwapchain.Deinit();

	ForRange(u32, i, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES)
	{
		gGlobal.mStaticDescriptors[i].Deinit();
	}

	gGlobal.mGraphicsQueue.Deinit();
	gGlobal.mDevice.Deinit();
}
