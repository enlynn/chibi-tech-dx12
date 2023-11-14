#include "simple_renderer.h"

#include "dx12/gpu_state.h"
#include "dx12/gpu_shader_utils.h"
#include "dx12/gpu_utils.h"
#include "dx12/gpu_render_pass.h"

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

// Example Implementation of a renderpass
class scene_pass : public render_pass
{
public:
    scene_pass() = default;

    virtual void OnInit(struct gpu_frame_cache* FrameCache)   override;
    virtual void OnDeinit(struct gpu_frame_cache* FrameCache) override;
    virtual void OnRender(struct gpu_frame_cache* FrameCache) override;

private:

    static constexpr const char* ShaderName        = "TestTriangle";
    static constexpr const char* RootSignatureName = "Test Triangle Name";
};

class resolve_pass : public render_pass
{
public:
    resolve_pass() = default;

    virtual void OnInit(struct gpu_frame_cache* FrameCache)   override;
    virtual void OnDeinit(struct gpu_frame_cache* FrameCache) override;
    virtual void OnRender(struct gpu_frame_cache* FrameCache) override;

private:
    static constexpr const char* ShaderName        = "TestTriangle";
    static constexpr const char* RootSignatureName = "Test Triangle Name";
};

//
// template<typename Id>
// struct id_generator {
//
//      u32 mFreeIndices; //
// }
//
//

#include <util/id.h>
#include <util/bit.h>

DEFINE_TYPE_ID(render_mesh_id);

enum class render_mesh_state
{
    unallocated,       // the render mesh is unallocated and is a free slot
    pending_uploaded,  // the render mesh has been initialized, but not queued yet for uploads
    uploading,         // the render mesh is uploading to the GPU, but not ready for use
    active,            // the render mesh is resident on the GPU and ready for rendering
    hidden,            // the render mesh is resident on the GPU but hidden and is inactive. Is not considered for rendering
};

// For now, there are no mesh Flags but I want to prototype a "Filter"
enum render_mesh_flags
{
    render_mesh_flag_none       = 0x00,
    render_mesh_flag_renderable = BitMaskU8(1),
};

struct render_draw_data
{
    gpu_resource            mVertexBufferResource = {};
    gpu_resource            mIndexBufferResource  = {};
    D3D12_INDEX_BUFFER_VIEW mIndexBufferView      = {};
    u32                     mElementCount         = 0;
};

class render_mesh_manger
{
public:
    render_mesh_manger();

    render_mesh_id AcquireMesh();          // Acquire a new id for a mesh. Guarenteed to be unique.
    void ReleaseMesh(render_mesh_id Id);   // Release a mesh id. The data for the mesh is removed and unallocated.

    void SetMeshState(render_mesh_id Id, render_mesh_state State);
    render_mesh_state GetMeshState(render_mesh_id Id) const;

private:
    static constexpr int cTotalAllowedMeshes = 10;
    using render_mesh_generator = id_generator<render_mesh_id, cTotalAllowedMeshes>;

    struct mesh_metadata
    { // Small header for mesh data
        render_mesh_state mState = render_mesh_state::unallocated;
        u8                mFlags = render_mesh_flag_none;
    };

    render_mesh_generator mIdGenerator = {};
    mesh_metadata         mMeshStates[cTotalAllowedMeshes];
    render_draw_data      mMeshData[cTotalAllowedMeshes];
};

//
// Render Mesh System Workflow
//
// render_mesh_id NewMesh = RenderMeshSystem->MeshAcquire();
//
// FrameCache->AddMeshUpload(NewMesh, GpuData);
// RenderMeshSystem->SetMeshState(NewMesh, render_mesh_state::pending_uploaded);
//
// //... later when the frame_cache is submitting uploads
// RenderMeshSystem->SetMeshState(NewMesh, render_mesh_state::uploading);
//
// //... later after the frame_cache is reset and we can guarantee the mesh is on the gpu
// render_upload_data Upload = ...;
//
// RenderMeshSystem->SetDrawData(Upload.MeshId, Upload.DrawData);
// RenderMeshSystem->SetMeshState(Upload.MeshId, render_mesh_state::active);
//
// // ... later when drawing
// u32 MeshFilterFlags = render_mesh_flags::none;
// u32 RenderMeshSystem->BuildDrawList(FrameCache->DrawList, MeshFilterFlags);
//

var_global constexpr bool  cEnableMSAA = true;

var_global gpu_state       gGlobal  = {};
// Temporary
var_global gpu_buffer      gVertexResource  = {};
var_global gpu_buffer      gIndexResource   = {};
var_global scene_pass      gScenePass       = {};
var_global resolve_pass    gResolvePass     = {};

struct complex_vertex
{
    f32x3 Pos;
    f32x3 Norm;
    f32x2 Tex;
};

struct test_cube
{
    complex_vertex Vertices[24];
    u16            Indices[36];
};

template<typename V, typename I> void
ReverseWinding(I *Indices, u32 IndexCount, V *Vertices, u32 VertexCount)
{
    for (u32 i = 0; i < IndexCount; i += 3)
    {
        I _x = Indices[i];
        I _y = Indices[i+2];

        Indices[i]   = _y;
        Indices[i+2] = _x;
    }

    for (u32 i = 0; i < VertexCount; ++i)
    {
        Vertices[i].Tex.X = (1.0f - Vertices[i].Tex.X);
    }
}

test_cube MakeCube(f32 Size = 1.0f, bool ShouldReverseWinding = false, bool InvertNormals = false)
{
    // 8 edges of cube.
    f32x3 p[8] = {
        {  Size, Size, -Size }, {  Size, Size,  Size }, {  Size, -Size,  Size }, {  Size, -Size, -Size },
        { -Size, Size,  Size }, { -Size, Size, -Size }, { -Size, -Size, -Size }, { -Size, -Size,  Size }
    };
    // 6 face normals
    f32x3 n[6] = { { 1, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 } };
    // 4 unique texture coordinates
    f32x2 t[4] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };

    // Indices for the vertex positions.
    u16 i[24] = {
        0, 1, 2, 3,  // +X
        4, 5, 6, 7,  // -X
        4, 1, 0, 5,  // +Y
        2, 7, 6, 3,  // -Y
        1, 4, 7, 2,  // +Z
        5, 0, 3, 6   // -Z
    };

    test_cube Result = {};
    memset(Result.Vertices, 0, sizeof(Result.Vertices));
    memset(Result.Indices, 0, sizeof(Result.Indices));

    if (InvertNormals)
    {
        for (auto & f : n)  // For each face of the cube.
        {
            f *= -1.0f;
        }
    }

    u16 vidx = 0;
    u16 iidx = 0;
    for (u16 f = 0; f < 6; ++f )  // For each face of the cube.
    {
        // Four vertices per face.
        Result.Vertices[vidx++] = { p[i[f * 4 + 0]], n[f], t[0] };
        Result.Vertices[vidx++] = { p[i[f * 4 + 1]], n[f], t[1] };
        Result.Vertices[vidx++] = { p[i[f * 4 + 2]], n[f], t[2] };
        Result.Vertices[vidx++] = { p[i[f * 4 + 3]], n[f], t[3] };

        // First triangle.
        Result.Indices[iidx++] =  f * 4 + 0;
        Result.Indices[iidx++] =  f * 4 + 1;
        Result.Indices[iidx++] =  f * 4 + 2;

        // Second triangle.
        Result.Indices[iidx++] =  f * 4 + 2;
        Result.Indices[iidx++] =  f * 4 + 3;
        Result.Indices[iidx++] =  f * 4 + 0;
    }

    if (ShouldReverseWinding)
    {
        ReverseWinding(Result.Indices, _countof(Result.Indices), Result.Vertices, _countof(Result.Vertices));
    }

    return Result;
}

void scene_pass::OnInit(gpu_frame_cache* FrameCache)
{
    auto VertexShader = shader_resource(shader_stage::vertex);
    auto PixelShader  = shader_resource(shader_stage::pixel);

    FrameCache->LoadShader(&VertexShader, "TestTriangle");
    FrameCache->LoadShader(&PixelShader,  "TestTriangle");

    { // Create a simple root signature
        gpu_root_descriptor VertexBuffers[]    = { { .mRootIndex = u32(triangle_root_parameter::vertex_buffer), .mType = gpu_descriptor_type::srv } };
        gpu_root_constant   PerDrawConstants[] = { { .mRootIndex = u32(triangle_root_parameter::per_draw), .mNum32bitValues = sizeof(vertex_draw_constants) / 4 } };
        const char*         DebugName          = "Scene Pass Root Signature";

        gpu_root_signature_info Info = {};
        Info.Descriptors         = farray(VertexBuffers, ArrayCount(VertexBuffers));
        Info.DescriptorConstants = farray(PerDrawConstants, ArrayCount(PerDrawConstants));
        Info.Name                = DebugName;
        mRootSignature = gpu_root_signature(*FrameCache->GetDevice(), Info);
    }

    DXGI_FORMAT RTFormat = gGlobal.mSwapchain.GetSwapchainFormat();

    { // Create a simple pso
        D3D12_RT_FORMAT_ARRAY PsoRTFormats = {};
        PsoRTFormats.NumRenderTargets = 1;
        PsoRTFormats.RTFormats[0]     = RTFormat;

        u32 SampleCount   = 1;
        u32 SampleQuality = 0;
        if (cEnableMSAA)
        {
            gpu_device* Device = FrameCache->GetDevice();

            DXGI_SAMPLE_DESC Samples = Device->GetMultisampleQualityLevels(RTFormat);
            SampleCount   = Samples.Count;
            SampleQuality = Samples.Quality;
        }

        gpu_graphics_pso_builder Builder = gpu_graphics_pso_builder::Builder();
        Builder.SetRootSignature(&mRootSignature)
            .SetVertexShader(&VertexShader)
            .SetPixelShader(&PixelShader)
            .SetRenderTargetFormats(1, { gGlobal.mSwapchain.GetSwapchainFormat() })
            .SetSampleQuality(SampleCount, SampleQuality);

        mPso = Builder.Compile(FrameCache);
    }
}

void scene_pass::OnDeinit(gpu_frame_cache* FrameCache)
{
    mPso.Release();
    mRootSignature.Release();
    mRenderTarget.Reset();
}

void scene_pass::OnRender(gpu_frame_cache* FrameCache)
{
    mRenderTarget.Reset();

    gpu_texture* SceneFramebuffer = FrameCache->GetFramebuffer(gpu_framebuffer_binding::main_color);
    mRenderTarget.AttachTexture(attachment_point::color0, SceneFramebuffer);

    gpu_command_list* CommandList = FrameCache->GetGraphicsCommandList();

    // Clear the Framebuffer and bind the render target.
    FrameCache->TransitionResource(SceneFramebuffer->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);

    f32x4 ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    CommandList->BindRenderTarget(&mRenderTarget, &ClearColor, false);

    // Scissor / Viewport
    D3D12_VIEWPORT Viewport = mRenderTarget.GetViewport();
    CommandList->SetViewport(Viewport);

    D3D12_RECT ScissorRect = {};
    ScissorRect.left   = 0;
    ScissorRect.top    = 0;
    ScissorRect.right  = (LONG)Viewport.Width;
    ScissorRect.bottom = (LONG)Viewport.Height;
    CommandList->SetScissorRect(ScissorRect);

    // Draw some geomtry

    CommandList->SetPipelineState(mPso);
    CommandList->SetGraphicsRootSignature(mRootSignature);

    CommandList->SetTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    CommandList->SetIndexBuffer(gIndexResource.GetIndexBufferView());
    CommandList->SetShaderResourceViewInline(u32(triangle_root_parameter::vertex_buffer), gVertexResource.GetGPUResource());

    vertex_draw_constants Constants = {};
    CommandList->SetGraphics32BitConstants(u32(triangle_root_parameter::per_draw), &Constants);

    FrameCache->FlushResourceBarriers(CommandList); // Flush barriers before drawing

    CommandList->DrawIndexedInstanced(gIndexResource.GetIndexCount());
}

void resolve_pass::OnInit(gpu_frame_cache* FrameCache)   {} // Does not need to initialize frame resources
void resolve_pass::OnDeinit(gpu_frame_cache* FrameCache) {} // No Resources to clean up

void resolve_pass::OnRender(gpu_frame_cache* FrameCache)
{
    gpu_command_list* CommandList = FrameCache->GetGraphicsCommandList();

    gpu_render_target* SwapchainRenderTarget = FrameCache->mGlobal->mSwapchain.GetRenderTarget();

    const gpu_texture* SwapchainBackBuffer = SwapchainRenderTarget->GetTexture(attachment_point::color0);
    const gpu_texture* SceneTexture        = FrameCache->GetFramebuffer(gpu_framebuffer_binding::main_color);

    bool MSAAEnabled = SceneTexture->GetResourceDesc().SampleDesc.Count > 1;
    if (MSAAEnabled)
    {
        CommandList->ResolveSubresource(FrameCache, SwapchainBackBuffer->GetResource(), SceneTexture->GetResource());
    }
    else
    {
        CommandList->CopyResource(FrameCache, SwapchainBackBuffer->GetResource(), SceneTexture->GetResource());
    }

    FrameCache->TransitionResource(SwapchainBackBuffer->GetResource(), D3D12_RESOURCE_STATE_PRESENT);
    FrameCache->FlushResourceBarriers(CommandList);
}

gpu_texture CreateFramebufferImage(gpu_frame_cache* FrameCache)
{
    gpu_swapchain* Swapchain = &FrameCache->mGlobal->mSwapchain;

    // In the future, will want to make this variable.
    DXGI_FORMAT FramebufferFormat = Swapchain->GetSwapchainFormat();

    // For now, these will match the dimensions of the swapchain
    u32 FramebufferWidth, FramebufferHeight;
    Swapchain->GetDimensions(FramebufferWidth, FramebufferHeight);

    u32 SampleCount   = 1;
    u32 SampleQuality = 0;
    if (cEnableMSAA)
    {
        gpu_device* Device = FrameCache->GetDevice();

        DXGI_SAMPLE_DESC Samples = Device->GetMultisampleQualityLevels(FramebufferFormat);
        SampleCount   = Samples.Count;
        SampleQuality = Samples.Quality;
    }

    D3D12_RESOURCE_DESC ImageDesc = GetTex2DDesc(
        FramebufferFormat, FramebufferWidth, FramebufferHeight,
        1, 1, SampleCount, SampleQuality);

    // Allow UAV in case a Renderpass wants to read from the Image
    ImageDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET|D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    D3D12_CLEAR_VALUE ClearValue = {};
    ClearValue.Format   = ImageDesc.Format;
    ClearValue.Color[0] = 0.0f;
    ClearValue.Color[1] = 0.0f;
    ClearValue.Color[2] = 0.0f;
    ClearValue.Color[3] = 1.0f;

    gpu_resource ImageResource = gpu_resource(*FrameCache->GetDevice(), ImageDesc, ClearValue);
    return gpu_texture(FrameCache, ImageResource);
}

void
SimpleRendererInit(simple_renderer_info& RenderInfo)
{
    if (RenderInfo.HeapAllocator)
		gGlobal.mHeapAllocator = RenderInfo.HeapAllocator->Clone();

    gGlobal.mResourceSystem = &RenderInfo.ResourceSystem;

	gGlobal.mDevice.Init();
	gGlobal.mGraphicsQueue = gpu_command_queue(gGlobal.mHeapAllocator, gpu_command_queue_type::graphics, &gGlobal.mDevice);

    gGlobal.mGlobalResourceState.mKnownStates = gpu_resource_state_map(gGlobal.mHeapAllocator, 10);

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
        gGlobal.mPerFrameCache[i].mResourceStateTracker = gpu_resource_state_tracker(gGlobal.mHeapAllocator);
    }

    gpu_frame_cache* FrameCache = gGlobal.GetFrameCache();

    //
    // Swapchain Setup
    //

	gGlobal.mSwapchain = gpu_swapchain(FrameCache, SwapchainInfo, RenderInfo.ClientWindow);

    //
    // Iterate over the frame cache again and create the simple framebuffers
    //

    ForRange(int, i, gpu_state::cMaxFrameCache)
    {
        gpu_frame_cache& Cache = gGlobal.mPerFrameCache[i];
        ForRange(int, FramebufferIndex, int(gpu_framebuffer_binding::max))
        {
            Cache.mFramebuffers[FramebufferIndex] = CreateFramebufferImage(FrameCache);
        }
    }

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
    FrameCache->FlushGPU(); // Forcibly upload all of the geometry (for now)

	//
	// Register the builtin-shader resource
	//

	resource_loader ShaderLoader = shader_resource::GetResourceLoader();
    gGlobal.mResourceSystem->RegisterLoader(ShaderLoader);

    //
    // Initialize Render Pass
    //

    gScenePass.OnInit(FrameCache);
    gResolvePass.OnInit(FrameCache);

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

    //------------------------------------------------------------------------------------------------------------------
    // Begin Frame Cache Cleanup

    // Reset the Per-Frame State Tracker
    FrameCache->mResourceStateTracker.Reset();
	
	// Update any pending command lists
	gGlobal.mGraphicsQueue.ProcessCommandLists();

	// Clean up any previous resources
	ReleaseStateResources(FrameCache->mStaleResources);

    // End Frame Cache Cleanup
    //------------------------------------------------------------------------------------------------------------------
    // Begin Rendering

    gScenePass.OnRender(FrameCache);    // Render the Triangle
    gResolvePass.OnRender(FrameCache);  // Copy Scene Texture into the Swapchain Image

	FrameCache->SubmitGraphicsCommandList();

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

	gScenePass.OnDeinit(gGlobal.GetFrameCache());
    gResolvePass.OnDeinit(gGlobal.GetFrameCache());

    gGlobal.mSwapchain.Release(gGlobal.GetFrameCache());

	// Free any stale resources
	ForRange(int, i, gpu_state::cMaxFrameCache)
	{
		ReleaseStateResources(gGlobal.mPerFrameCache[i].mStaleResources);

        gpu_frame_cache& Cache = gGlobal.mPerFrameCache[i];
        ForRange(int, FramebufferIndex, int(gpu_framebuffer_binding::max))
        {
            Cache.mFramebuffers[FramebufferIndex].ReleaseUnsafe(&Cache);
        }
	}

	ForRange(u32, i, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES)
	{
		gGlobal.mStaticDescriptors[i].Deinit();
	}

	gGlobal.mGraphicsQueue.Deinit();
	gGlobal.mDevice.Deinit();
}
