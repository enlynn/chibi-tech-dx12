#pragma once

#include <types.h>
#include <util/str8.h>

#include "d3d12_common.h"
#include "renderer/dx12/d3d12_common.h"
#include "renderer/dx12/gpu_root_signature.h"

enum class gpu_fill_mode : u8
{
    wireframe,
    solid,
};
 
enum class gpu_cull_mode  : u8
{
    none,
    front,
    back,
};

enum class gpu_format : u8
{
    r32_float,
    r32g32_float,
    r32g32b32_float,
    r32g32b32a32_float,
    r8g8b8a8_unorm,
    
    // TODO(enlynn): Others on a as-needed basis
    
    d32_float, // depth format
};

enum class gpu_input_class : u8
{
    per_vertex,
    per_instance,
};

enum class gpu_topology
{
    undefined,
    point,
    line,
    triangle,
    patch,
};

/* Specifies multisampling settings */
struct gpu_sample_desc
{
    u32 mCount   = 1;
    u32 mQuality = 0;
};

struct gpu_rasterizer
{
    gpu_fill_mode mFillMode              = gpu_fill_mode::solid;
    gpu_cull_mode mCullMode              = gpu_cull_mode::none;
    bool          mFrontCounterClockwise = true;
    /* Omit depth bias? */
    /* Omit depth bias clamp? */
    /* Omit slope scaled depth bias? */
    bool          mDepthClipEnabled      = false;
    bool          mMultiSampleEnabled    = false;
    bool          mAntiAliasedEnabled    = false;
    u32           mForcedSampleCount     = false;
    /* Omit conservative raster mode */
};

struct gpu_input_element_desc
{
    // 
    // Acceptable Semantic Names: 
    // - BINORMAL[n]      | Binormal                         | float4
    // - BLENDINDICES[n]  | Blend indices                    | uint
    // - BLENDWEIGHT[n]   | Blend weights                    | float
    // - COLOR[n]         | Diffuse and specular color       | float4
    // - NORMAL[n]        | Normal vector                    | float4
    // - POSITION[n]      | Vertex position in object space. | float4
    // - POSITIONT        | Transformed vertex position.     | float4
    // - PSIZE[n]         | Point size                       | float
    // - TANGENT[n]       | Tangent                          | float4
    // - TEXCOORD[n]      | Texture Coordinates              | float4
    // 
    // [n] is an optional integer between 0 and the number of resources supported. For example, POSITION0, TEXCOORD1.
    //
    istr8            SemanticName      = "POSITION0";
    u32              SemanticIndex     = 0;
    gpu_format       Format            = gpu_format::r32g32b32_float;
    u32              InputSlot         = 0;
    u32              AlignedByteOffset = 0;
    bool             IsPerVertex       = false; // If false then is Per Instance
    u32              InputStepRate     = 0;
};

enum class gpu_raster_state : u8
{
    default_raster,
    default_msaa,
    default_cw,
    default_cw_msaa,
    two_sided,
    two_sided_msaa,
    shadow,
    shadow_cw,
    shadow_two_sided,
};

enum class gpu_depth_stencil_state : u8
{
    disabled,
    read_write,
    read_only,
    read_only_reversed,
    test_equal,
};

enum class gpu_blend_state : u8
{
    disabled,
    no_color_write,
    traditional,
    pre_multiplied,
    additive,
    tradition_additive,
};

// Get a default description of the rasterizer state
D3D12_RASTERIZER_DESC GetRasterizerState(gpu_raster_state Type);
// Get a default description of the depth-stencil state
D3D12_DEPTH_STENCIL_DESC GetDepthStencilState(gpu_depth_stencil_state Type);
// Get a default description of the blend state
D3D12_BLEND_DESC GetBlendState(gpu_blend_state Type);

struct gpu_pso_info
{
    const class gpu_root_signature& mRootSignature;
    // Shaders used for this pipeline
    D3D12_SHADER_BYTECODE           mVertexShader        = {};
    D3D12_SHADER_BYTECODE           mPixelShader         = {};
    // List of Render Targets used for this pipeline
    D3D12_RT_FORMAT_ARRAY           mRenderTargetFormats = {};
    D3D12_RASTERIZER_DESC           mRasterizer          = GetRasterizerState(gpu_raster_state::default_raster);
    D3D12_DEPTH_STENCIL_DESC        mDepth               = GetDepthStencilState(gpu_depth_stencil_state::disabled);
    D3D12_BLEND_DESC                mBlend               = GetBlendState(gpu_blend_state::disabled);
    // Vertex Topology
    gpu_topology                    mTopology            = gpu_topology::triangle;
    // Optional, only needed if depth stencil state is used
    DXGI_FORMAT                     mDepthFormat         = DXGI_FORMAT_UNKNOWN;
    // Optional, only needed if using multisampling
    gpu_sample_desc                 mSampleDesc          = {};
    // List of Vertex Inputs for the pipeline. Since bindless is preferred, this will usually be empty.
    farray<gpu_input_element_desc>  mInputElements       = {};
};

class gpu_pso 
{
public:
    gpu_pso() = default;
    gpu_pso(class gpu_device& Device, gpu_pso_info& Info);

    void Release() { if (mHandle) ComSafeRelease(mHandle); }

    struct ID3D12PipelineState* AsHandle() const { return mHandle; }

private:
    struct ID3D12PipelineState *mHandle = nullptr;
};