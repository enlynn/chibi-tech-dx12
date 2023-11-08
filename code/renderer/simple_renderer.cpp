#include "simple_renderer.h"

#include "dx12/gpu_state.h"
#include "dx12/gpu_shader_utils.h"

#include <types.h>
#include <util/allocator.h>
#include <platform/platform.h>
#include <systems/resource_system.h>

enum class triangle_root_parameter
{
	vertex_buffer = 0,
	per_draw      = 1,
};

struct vertex_draw_constants
{
	u32 uVertexOffset      = 0;
	u32 uVertexBufferIndex = 0;
};

var_global gpu_state       gGlobal  = {};
// Temporary
gpu_buffer         gVertexResource  = {};
gpu_buffer         gIndexResource   = {};
gpu_root_signature gSimpleSignature = {};
gpu_pso            gSimplePipeline  = {};

class gpu_render_pass
{
    gpu_root_signature mRootSignature;
    gpu_pso            mPso;

    DXGI_FORMAT        mRenderTargetFormats[8];
    u32                mRenderTargetCount;

    gpu_render_target  mRenderTarget;
};

void
SimpleRendererInit(simple_renderer_info& RenderInfo)
{
    if (RenderInfo.HeapAllocator)
		gGlobal.mHeapAllocator = RenderInfo.HeapAllocator->Clone();

	gGlobal.mDevice.Init();
	gGlobal.mGraphicsQueue = gpu_command_queue(gGlobal.mHeapAllocator, gpu_command_queue_type::graphics, &gGlobal.mDevice);

	// Create the CPU Descriptor Allocators
	ForRange(u32, i, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES)
	{
		gGlobal.mStaticDescriptors[i].Init(&gGlobal.mDevice, gGlobal.mHeapAllocator, (D3D12_DESCRIPTOR_HEAP_TYPE)i);
	}

	gpu_swapchain_info SwapchainInfo = {
		.mDevice                    = &gGlobal.mDevice,
		.mPresentQueue              = &gGlobal.mGraphicsQueue,
		.mRenderTargetDesciptorHeap = &gGlobal.mStaticDescriptors[D3D12_DESCRIPTOR_HEAP_TYPE_RTV],
		// Leave everything else at their defaults for now...
	};

    //
    // Let's setup the the Per Frame Resources and let Frame 0 be the setup frame
    //

    gGlobal.mPerFrameCache = gGlobal.mHeapAllocator.AllocArray<gpu_frame_cache>(gpu_state::cMaxFrameCache, allocation_strategy::default_init);
    ForRange(int, i, gpu_state::cMaxFrameCache)
    {
        gGlobal.mPerFrameCache[i].mGlobal         = &gGlobal;
        gGlobal.mPerFrameCache[i].mStaleResources = darray<gpu_resource>(gGlobal.mHeapAllocator, 5);
    }

    gpu_frame_cache* FrameCache = gGlobal.GetFrameCache();

    //
    // Swapchain Setup
    //

	gGlobal.mSwapchain = gpu_swapchain(FrameCache, SwapchainInfo, RenderInfo.ClientWindow);

	//
	// Test Geometry
	//

	// Let's create a test resource
	struct vertex { f32 Pos[2]; f32 Col[3]; };
	vertex Vertices[] = {
		{ .Pos = { -0.5f, -0.5f }, .Col = { 1.0f, 1.0f, 1.0f  } },
		{ .Pos = {  0.5f, -0.5f }, .Col = { 0.36f,0.81f,0.98f } },
		{ .Pos = {  0.0f,  0.5f }, .Col = { 0.96f,0.66f,0.72f } },
	};

	u16 Indices[] = { 0, 1, 2 };

	gpu_command_list* UploadList = FrameCache->GetCopyCommandList();

    gpu_byte_address_buffer_info VertexInfo = {};
    VertexInfo.mCount                       = ArrayCount(Vertices);
    VertexInfo.mStride                      = sizeof(vertex);
    VertexInfo.mData                        = Vertices;
    gVertexResource = gpu_buffer::CreateByteAdressBuffer(FrameCache, VertexInfo);

    gpu_index_buffer_info IndexInfo = {};
    IndexInfo.mIndexCount = 3;
    IndexInfo.mIsU16 = true;
    IndexInfo.mIndices = Indices;
    gIndexResource = gpu_buffer::CreateIndexBuffer(FrameCache, IndexInfo);

    FrameCache->SubmitCopyCommandList();
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
		gpu_root_descriptor VertexBuffers[]    = { { .mRootIndex = u32(triangle_root_parameter::vertex_buffer), .mType = gpu_descriptor_type::srv } };
		gpu_root_constant   PerDrawConstants[] = { { .mRootIndex = u32(triangle_root_parameter::per_draw), .mNum32bitValues = sizeof(vertex_draw_constants) / 4 } };
		const char*         DebugName          = "Simple Root Signature";

		gpu_root_signature_info Info = {};
		Info.Descriptors         = farray(VertexBuffers, ArrayCount(VertexBuffers));
		Info.DescriptorConstants = farray(PerDrawConstants, ArrayCount(PerDrawConstants));
		Info.Name                = DebugName;
		gSimpleSignature = gpu_root_signature(gGlobal.mDevice, Info);
	}

	{ // Create a simple pso
		gpu_pso_info Info = { .mRootSignature = gSimpleSignature };
		Info.mVertexShader = VertexShader.GetShaderBytecode();
		Info.mPixelShader  = PixelShader.GetShaderBytecode();

		D3D12_RT_FORMAT_ARRAY PsoRTFormats = {};
		PsoRTFormats.NumRenderTargets = 1;
		PsoRTFormats.RTFormats[0]     = gGlobal.mSwapchain.GetSwapchainFormat();
		Info.mRenderTargetFormats     = PsoRTFormats;

		gSimplePipeline = gpu_pso(gGlobal.mDevice, Info);
	}

	gGlobal.mFrameCount += 1;
}

fn_internal void
ReleaseStateResources(darray<gpu_resource>& StaleResources)
{
	for (gpu_resource& Resource : StaleResources)
	{
		ID3D12Resource* ResourceHandle = Resource.AsHandle();
		ComSafeRelease(ResourceHandle);
	}
	StaleResources.Clear();
}

void
SimpleRendererRender()
{
	gpu_frame_cache* FrameCache = gGlobal.GetFrameCache();
	
	// Update any pending command lists
	gGlobal.mGraphicsQueue.ProcessCommandLists();

	// Clean up any previous resources
	ReleaseStateResources(FrameCache->mStaleResources);

	// Let's render!
	gpu_render_target* MainRenderTarget = gGlobal.mSwapchain.GetRenderTarget();
	gpu_command_list*  List             = gGlobal.mGraphicsQueue.GetCommandList();

	f32x4 ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
	List->BindRenderTarget(MainRenderTarget, &ClearColor, false);

	// Scissor / Viewport
	D3D12_VIEWPORT Viewport = MainRenderTarget->GetViewport();
	List->SetViewport(Viewport);

	D3D12_RECT ScissorRect = {};
	ScissorRect.left   = 0;
	ScissorRect.top    = 0;
	ScissorRect.right  = (LONG)Viewport.Width;
	ScissorRect.bottom = (LONG)Viewport.Height;
	List->SetScissorRect(ScissorRect);

	// Draw some geomtry

	List->SetPipelineState(gSimplePipeline);
	List->SetGraphicsRootSignature(gSimpleSignature);

	List->SetTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	List->SetIndexBuffer(gIndexResource.GetIndexBufferView());
	List->SetShaderResourceViewInline(u32(triangle_root_parameter::vertex_buffer), gVertexResource.GetGPUResource());

	vertex_draw_constants Constants = {};
	List->SetGraphics32BitConstants(u32(triangle_root_parameter::per_draw), &Constants);

	List->DrawIndexedInstanced(gIndexResource.GetIndexCount());

    {
        const gpu_texture*  Backbuffer = MainRenderTarget->GetTexture(attachment_point::color0);
        const gpu_resource* Resource   = Backbuffer->GetResource();

        gpu_transition_barrier ToRenderBarrier = {};
        ToRenderBarrier.BeforeState = D3D12_RESOURCE_STATE_RENDER_TARGET;
        ToRenderBarrier.AfterState  = D3D12_RESOURCE_STATE_PRESENT;
        List->TransitionBarrier(*Resource, ToRenderBarrier);
    }

	gGlobal.mGraphicsQueue.ExecuteCommandLists(&List);

	gGlobal.mSwapchain.Present();

	gGlobal.mFrameCount += 1;
}

void
SimpleRendererDeinit()
{
	{ // Free the temp resources
        ID3D12Resource* ResourceHandle = gVertexResource.GetGPUResource()->AsHandle();
		ComSafeRelease(ResourceHandle);

		ResourceHandle = gIndexResource.GetGPUResource()->AsHandle();
		ComSafeRelease(ResourceHandle);
	}

	gSimplePipeline.Release();
	gSimpleSignature.Release();

    gGlobal.mSwapchain.Release(gGlobal.GetFrameCache());

	// Free any stale resources
	ForRange(int, i, gpu_state::cMaxFrameCache)
	{
		ReleaseStateResources(gGlobal.mPerFrameCache[i].mStaleResources);
	}

	ForRange(u32, i, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES)
	{
		gGlobal.mStaticDescriptors[i].Deinit();
	}

	gGlobal.mGraphicsQueue.Deinit();
	gGlobal.mDevice.Deinit();
}
