#include "gpu_pso.h"
#include "gpu_device.h"
#include "gpu_root_signature.h"
#include "renderer/dx12/d3d12_common.h"
#include "renderer/dx12/gpu_pso.h"

// SOURCE for constants: 
// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/GraphicsCommon.cpp

var_global constexpr D3D12_RASTERIZER_DESC cRasterizerDefault = {
    D3D12_FILL_MODE_SOLID,                     // Fill Mode
    D3D12_CULL_MODE_NONE,                      // Cull mode
    TRUE,                                      // Winding order (true == ccw)
    D3D12_DEFAULT_DEPTH_BIAS,                  // Depth Bias
    D3D12_DEFAULT_DEPTH_BIAS_CLAMP,            // Depth Bias clamp 
    D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,     // Slope scaled depth bias
    TRUE,                                      // Depth Clip Enable
    FALSE,                                     // Multisample Enable
    FALSE,                                     // Antialiased enable
    0,                                         // Forced sample count
    D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF, // Conservative raster
};

var_global constexpr D3D12_RASTERIZER_DESC cRasterizerDefaultMsaa = {
    D3D12_FILL_MODE_SOLID,                     // Fill Mode
    D3D12_CULL_MODE_BACK,                      // Cull mode
    TRUE,                                      // Winding order (true == ccw)
    D3D12_DEFAULT_DEPTH_BIAS,                  // Depth Bias
    D3D12_DEFAULT_DEPTH_BIAS_CLAMP,            // Depth Bias clamp 
    D3D12_DEFAULT_DEPTH_BIAS,                  // Slope scaled depth bias
    TRUE,                                      // Depth Clip Enable
    TRUE,                                      // Multisample Enable
    FALSE,                                     // Antialiased enable
    0,                                         // Forced sample count
    D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF, // Conservative raster
};
    
    // Clockwise winding vertex winding
    
var_global constexpr D3D12_RASTERIZER_DESC cRasterizerDefaultCw = {
    D3D12_FILL_MODE_SOLID,                     // Fill Mode
    D3D12_CULL_MODE_NONE,                      // Cull mode
    FALSE,                                     // Winding order (true == ccw)
    D3D12_DEFAULT_DEPTH_BIAS,                  // Depth Bias
    D3D12_DEFAULT_DEPTH_BIAS_CLAMP,            // Depth Bias clamp 
    D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,     // Slope scaled depth bias
    TRUE,                                      // Depth Clip Enable
    FALSE,                                     // Multisample Enable
    FALSE,                                     // Antialiased enable
    0,                                         // Forced sample count
    D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF, // Conservative raster
};
    
var_global constexpr D3D12_RASTERIZER_DESC cRasterizerDefaultCwMsaa = {
    D3D12_FILL_MODE_SOLID,                     // Fill Mode
    D3D12_CULL_MODE_BACK,                      // Cull mode
    FALSE,                                     // Winding order (true == ccw)
    D3D12_DEFAULT_DEPTH_BIAS,                  // Depth Bias
    D3D12_DEFAULT_DEPTH_BIAS_CLAMP,            // Depth Bias clamp 
    D3D12_DEFAULT_DEPTH_BIAS,                  // Slope scaled depth bias
    TRUE,                                      // Depth Clip Enable
    TRUE,                                      // Multisample Enable
    FALSE,                                     // Antialiased enable
    0,                                         // Forced sample count
    D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF, // Conservative raster
};
    
    // CCW Two Sided
    
    var_global constexpr D3D12_RASTERIZER_DESC cRasterizerTwoSided = {
    D3D12_FILL_MODE_SOLID,                     // Fill Mode
    D3D12_CULL_MODE_NONE,                      // Cull mode
    TRUE,                                      // Winding order (true == ccw)
    D3D12_DEFAULT_DEPTH_BIAS,                  // Depth Bias
    D3D12_DEFAULT_DEPTH_BIAS_CLAMP,            // Depth Bias clamp 
    D3D12_DEFAULT_DEPTH_BIAS,                  // Slope scaled depth bias
    TRUE,                                      // Depth Clip Enable
    FALSE,                                     // Multisample Enable
    FALSE,                                     // Antialiased enable
    0,                                         // Forced sample count
    D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF, // Conservative raster
    };
    
var_global constexpr D3D12_RASTERIZER_DESC cRasterizerTwoSidedMsaa = {
    D3D12_FILL_MODE_SOLID,                     // Fill Mode
    D3D12_CULL_MODE_NONE,                      // Cull mode
    TRUE,                                      // Winding order (true == ccw)
    D3D12_DEFAULT_DEPTH_BIAS,                  // Depth Bias
    D3D12_DEFAULT_DEPTH_BIAS_CLAMP,            // Depth Bias clamp 
    D3D12_DEFAULT_DEPTH_BIAS,                  // Slope scaled depth bias
    TRUE,                                      // Depth Clip Enable
    TRUE,                                      // Multisample Enable
    FALSE,                                     // Antialiased enable
    0,                                         // Forced sample count
    D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF, // Conservative raster
};
    
var_global constexpr D3D12_RASTERIZER_DESC cRasterizerShadow = {
    D3D12_FILL_MODE_SOLID,                     // Fill Mode
    D3D12_CULL_MODE_BACK,                      // Cull mode
    TRUE,                                      // Winding order (true == ccw)
    -100,                                      // Depth Bias
    D3D12_DEFAULT_DEPTH_BIAS_CLAMP,            // Depth Bias clamp 
    -1.5,                                      // Slope scaled depth bias
    TRUE,                                      // Depth Clip Enable
    FALSE,                                     // Multisample Enable
    FALSE,                                     // Antialiased enable
    0,                                         // Forced sample count
    D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF, // Conservative raster
};
    
var_global constexpr D3D12_RASTERIZER_DESC cRasterizerShadowCW = {
    D3D12_FILL_MODE_SOLID,                     // Fill Mode
    D3D12_CULL_MODE_BACK,                      // Cull mode
    FALSE,                                     // Winding order (true == ccw)
    -100,                                      // Depth Bias
    D3D12_DEFAULT_DEPTH_BIAS_CLAMP,            // Depth Bias clamp 
    -1.5,                                      // Slope scaled depth bias
    TRUE,                                      // Depth Clip Enable
    FALSE,                                     // Multisample Enable
    FALSE,                                     // Antialiased enable
    0,                                         // Forced sample count
    D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF, // Conservative raster
};
    
var_global constexpr D3D12_RASTERIZER_DESC cRasterizerShadowTwoSided = {
    D3D12_FILL_MODE_SOLID,                     // Fill Mode
    D3D12_CULL_MODE_NONE,                      // Cull mode
    TRUE,                                      // Winding order (true == ccw)
    -100,                                      // Depth Bias
    D3D12_DEFAULT_DEPTH_BIAS_CLAMP,            // Depth Bias clamp 
    -1.5,                                      // Slope scaled depth bias
    TRUE,                                      // Depth Clip Enable
    FALSE,                                     // Multisample Enable
    FALSE,                                     // Antialiased enable
    0,                                         // Forced sample count
    D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF, // Conservative raster
};

var_global constexpr D3D12_DEPTH_STENCIL_DESC cDepthStateDisabled = {
    FALSE,                            // DepthEnable
    D3D12_DEPTH_WRITE_MASK_ZERO,      // DepthWriteMask
    D3D12_COMPARISON_FUNC_ALWAYS,     // DepthFunc
    FALSE,                            // StencilEnable
    D3D12_DEFAULT_STENCIL_READ_MASK,  // StencilReadMask
    D3D12_DEFAULT_STENCIL_WRITE_MASK, // StencilWriteMask
    {                                 // FrontFace
        D3D12_STENCIL_OP_KEEP,            // StencilFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilDepthFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilPassOp
        D3D12_COMPARISON_FUNC_ALWAYS,     // StencilFunc;
    },
    {                                 // BackFace
        D3D12_STENCIL_OP_KEEP,            // StencilFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilDepthFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilPassOp
        D3D12_COMPARISON_FUNC_ALWAYS,     // StencilFunc;
    },
};

var_global constexpr D3D12_DEPTH_STENCIL_DESC cDepthStateReadWrite = {
    TRUE,                             // DepthEnable
    D3D12_DEPTH_WRITE_MASK_ALL,       // DepthWriteMask
    D3D12_COMPARISON_FUNC_LESS,       // DepthFunc
    FALSE,                            // StencilEnable
    D3D12_DEFAULT_STENCIL_READ_MASK,  // StencilReadMask
    D3D12_DEFAULT_STENCIL_WRITE_MASK, // StencilWriteMask
    {                                 // FrontFace
        D3D12_STENCIL_OP_KEEP,            // StencilFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilDepthFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilPassOp
        D3D12_COMPARISON_FUNC_ALWAYS,     // StencilFunc;
    },
    {                                 // BackFace
        D3D12_STENCIL_OP_KEEP,            // StencilFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilDepthFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilPassOp
        D3D12_COMPARISON_FUNC_ALWAYS,     // StencilFunc;
    },
};

var_global constexpr D3D12_DEPTH_STENCIL_DESC cDepthStateReadOnly = {
    TRUE,                             // DepthEnable
    D3D12_DEPTH_WRITE_MASK_ZERO,      // DepthWriteMask
    D3D12_COMPARISON_FUNC_GREATER_EQUAL, // DepthFunc
    FALSE,                            // StencilEnable
    D3D12_DEFAULT_STENCIL_READ_MASK,  // StencilReadMask
    D3D12_DEFAULT_STENCIL_WRITE_MASK, // StencilWriteMask
    {                                 // FrontFace
        D3D12_STENCIL_OP_KEEP,            // StencilFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilDepthFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilPassOp
        D3D12_COMPARISON_FUNC_ALWAYS,     // StencilFunc;
    },
    {                                 // BackFace
        D3D12_STENCIL_OP_KEEP,            // StencilFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilDepthFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilPassOp
        D3D12_COMPARISON_FUNC_ALWAYS,     // StencilFunc;
    },
};
    
var_global constexpr D3D12_DEPTH_STENCIL_DESC cDepthStateReadOnlyReversed = {
    TRUE,                             // DepthEnable
    D3D12_DEPTH_WRITE_MASK_ZERO,      // DepthWriteMask
    D3D12_COMPARISON_FUNC_LESS,       // DepthFunc
    FALSE,                            // StencilEnable
    D3D12_DEFAULT_STENCIL_READ_MASK,  // StencilReadMask
    D3D12_DEFAULT_STENCIL_WRITE_MASK, // StencilWriteMask
    {                                 // FrontFace
        D3D12_STENCIL_OP_KEEP,            // StencilFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilDepthFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilPassOp
        D3D12_COMPARISON_FUNC_ALWAYS,     // StencilFunc;
    },
    {                                 // BackFace
        D3D12_STENCIL_OP_KEEP,            // StencilFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilDepthFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilPassOp
        D3D12_COMPARISON_FUNC_ALWAYS,     // StencilFunc;
    },
};

var_global constexpr D3D12_DEPTH_STENCIL_DESC cDepthStateTestEqual = {
    TRUE,                             // DepthEnable
    D3D12_DEPTH_WRITE_MASK_ZERO,      // DepthWriteMask
    D3D12_COMPARISON_FUNC_EQUAL,      // DepthFunc
    FALSE,                            // StencilEnable
    D3D12_DEFAULT_STENCIL_READ_MASK,  // StencilReadMask
    D3D12_DEFAULT_STENCIL_WRITE_MASK, // StencilWriteMask
    {                                 // FrontFace
        D3D12_STENCIL_OP_KEEP,            // StencilFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilDepthFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilPassOp
        D3D12_COMPARISON_FUNC_ALWAYS,     // StencilFunc;
    },
    {                                 // BackFace
        D3D12_STENCIL_OP_KEEP,            // StencilFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilDepthFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilPassOp
        D3D12_COMPARISON_FUNC_ALWAYS,     // StencilFunc;
    },
};
    
// alpha blend
var_global constexpr D3D12_BLEND_DESC cBlendNoColorWrite = {
    FALSE,                             // AlphaToCoverageEnable
    FALSE,                             // IndependentBlendEnable
    {                                  // RenderTarget[8]
        {                                  // index = 0
            FALSE,                             // BlendEnable
            FALSE,                             // LogicOpEnable
            D3D12_BLEND_SRC_ALPHA,             // SrcBlend
            D3D12_BLEND_INV_SRC_ALPHA,         // DestBlend
            D3D12_BLEND_OP_ADD,                // BlendOp
            D3D12_BLEND_ONE,                   // SrcBlendAlpha
            D3D12_BLEND_INV_SRC_ALPHA,         // DestBlendAlpha
            D3D12_BLEND_OP_ADD,                // BlendOpAlpha
            D3D12_LOGIC_OP_NOOP,               // LogicOp
            0,                                 // RenderTargetWriteMask
        }, {}, {}, {}, {}, {}, {}, {} // Make sure to default init the other slots
    },
};
    
var_global constexpr D3D12_BLEND_DESC cBlendDisable = {
    FALSE,                             // AlphaToCoverageEnable
    FALSE,                             // IndependentBlendEnable
    {                                  // RenderTarget[8]
        {                                  // index = 0
            FALSE,                             // BlendEnable
            FALSE,                             // LogicOpEnable
            D3D12_BLEND_SRC_ALPHA,             // SrcBlend
            D3D12_BLEND_INV_SRC_ALPHA,         // DestBlend
            D3D12_BLEND_OP_ADD,                // BlendOp
            D3D12_BLEND_ONE,                   // SrcBlendAlpha
            D3D12_BLEND_INV_SRC_ALPHA,         // DestBlendAlpha
            D3D12_BLEND_OP_ADD,                // BlendOpAlpha
            D3D12_LOGIC_OP_NOOP,               // LogicOp
            D3D12_COLOR_WRITE_ENABLE_ALL,      // RenderTargetWriteMask
        }, {}, {}, {}, {}, {}, {}, {} // Make sure to default init the other slots
    },
};
    
var_global constexpr D3D12_BLEND_DESC cBlendTraditional = {
    FALSE,                             // AlphaToCoverageEnable
    FALSE,                             // IndependentBlendEnable
    {                                  // RenderTarget[8]
        {                                  // index = 0
            TRUE,                              // BlendEnable
            FALSE,                             // LogicOpEnable
            D3D12_BLEND_SRC_ALPHA,             // SrcBlend
            D3D12_BLEND_INV_SRC_ALPHA,         // DestBlend
            D3D12_BLEND_OP_ADD,                // BlendOp
            D3D12_BLEND_ONE,                   // SrcBlendAlpha
            D3D12_BLEND_INV_SRC_ALPHA,         // DestBlendAlpha
            D3D12_BLEND_OP_ADD,                // BlendOpAlpha
            D3D12_LOGIC_OP_NOOP,               // LogicOp
            D3D12_COLOR_WRITE_ENABLE_ALL,      // RenderTargetWriteMask
        }, {}, {}, {}, {}, {}, {}, {} // Make sure to default init the other slots
    },
};
    
var_global constexpr D3D12_BLEND_DESC cBlendPreMultiplied = {
    FALSE,                             // AlphaToCoverageEnable
    FALSE,                             // IndependentBlendEnable
    {                                  // RenderTarget[8]
        {                                  // index = 0
            TRUE,                              // BlendEnable
            FALSE,                             // LogicOpEnable
            D3D12_BLEND_ONE,                   // SrcBlend
            D3D12_BLEND_INV_SRC_ALPHA,         // DestBlend
            D3D12_BLEND_OP_ADD,                // BlendOp
            D3D12_BLEND_ONE,                   // SrcBlendAlpha
            D3D12_BLEND_INV_SRC_ALPHA,         // DestBlendAlpha
            D3D12_BLEND_OP_ADD,                // BlendOpAlpha
            D3D12_LOGIC_OP_NOOP,               // LogicOp
            D3D12_COLOR_WRITE_ENABLE_ALL,      // RenderTargetWriteMask
        }, {}, {}, {}, {}, {}, {}, {} // Make sure to default init the other slots
    },
};
    
var_global constexpr D3D12_BLEND_DESC cBlendAdditive = {
    FALSE,                             // AlphaToCoverageEnable
    FALSE,                             // IndependentBlendEnable
    {                                  // RenderTarget[8]
        {                                  // index = 0
            TRUE,                              // BlendEnable
            FALSE,                             // LogicOpEnable
            D3D12_BLEND_ONE,                   // SrcBlend
            D3D12_BLEND_ONE,                   // DestBlend
            D3D12_BLEND_OP_ADD,                // BlendOp
            D3D12_BLEND_ONE,                   // SrcBlendAlpha
            D3D12_BLEND_INV_SRC_ALPHA,         // DestBlendAlpha
            D3D12_BLEND_OP_ADD,                // BlendOpAlpha
            D3D12_LOGIC_OP_NOOP,               // LogicOp
            D3D12_COLOR_WRITE_ENABLE_ALL,      // RenderTargetWriteMask
        }, {}, {}, {}, {}, {}, {}, {} // Make sure to default init the other slots
    },
};
    
var_global constexpr D3D12_BLEND_DESC cBlendTraditionalAdditive = {
    FALSE,                             // AlphaToCoverageEnable
    FALSE,                             // IndependentBlendEnable
    {                                  // RenderTarget[8]
        {                                  // index = 0
            TRUE,                              // BlendEnable
            FALSE,                             // LogicOpEnable
            D3D12_BLEND_SRC_ALPHA,             // SrcBlend
            D3D12_BLEND_ONE,                   // DestBlend
            D3D12_BLEND_OP_ADD,                // BlendOp
            D3D12_BLEND_ONE,                   // SrcBlendAlpha
            D3D12_BLEND_INV_SRC_ALPHA,         // DestBlendAlpha
            D3D12_BLEND_OP_ADD,                // BlendOpAlpha
            D3D12_LOGIC_OP_NOOP,               // LogicOp
            D3D12_COLOR_WRITE_ENABLE_ALL,      // RenderTargetWriteMask
        }, {}, {}, {}, {}, {}, {}, {} // Make sure to default init the other slots
    },
};

// Get a default description of the rasterizer state
D3D12_RASTERIZER_DESC 
GetRasterizerState(gpu_raster_state type)
{
    D3D12_RASTERIZER_DESC Result = {};
    
    if (type == gpu_raster_state::default_raster)
        Result = cRasterizerDefault;
    else if (type == gpu_raster_state::default_msaa)
        Result = cRasterizerDefaultMsaa;
    else if (type == gpu_raster_state::default_cw)
        Result = cRasterizerDefaultCw;
    else if (type == gpu_raster_state::default_cw_msaa)
        Result = cRasterizerDefaultCwMsaa;
    else if (type == gpu_raster_state::two_sided)
        Result = cRasterizerTwoSided;
    else if (type == gpu_raster_state::two_sided_msaa)
        Result = cRasterizerTwoSidedMsaa;
    else if (type == gpu_raster_state::shadow)
        Result = cRasterizerShadow;
    else if (type == gpu_raster_state::shadow_cw)
        Result = cRasterizerShadowCW;
    else if (type == gpu_raster_state::shadow_two_sided)
        Result = cRasterizerShadowTwoSided;
    
    return Result;
}

// Get a default description of the depth-stencil state
D3D12_DEPTH_STENCIL_DESC 
GetDepthStencilState(gpu_depth_stencil_state type)
{
    D3D12_DEPTH_STENCIL_DESC Result = {};
    
    if (type == gpu_depth_stencil_state::disabled)
        Result = cDepthStateDisabled;
    else if (type == gpu_depth_stencil_state::read_write)
        Result = cDepthStateReadWrite;
    else if (type == gpu_depth_stencil_state::read_only)
        Result = cDepthStateReadOnly;
    else if (type == gpu_depth_stencil_state::read_only_reversed)
        Result = cDepthStateReadOnlyReversed;
    else if (type == gpu_depth_stencil_state::test_equal)
        Result = cDepthStateTestEqual;
    
    return Result;
}

// Get a default description of the blend state
D3D12_BLEND_DESC 
GetBlendState(gpu_blend_state type)
{
    D3D12_BLEND_DESC Result = {};
    
    if (type == gpu_blend_state::disabled)
        Result = cBlendDisable;
    else if (type == gpu_blend_state::no_color_write)
        Result = cBlendNoColorWrite;
    else if (type == gpu_blend_state::traditional)
        Result = cBlendTraditional;
    else if (type == gpu_blend_state::pre_multiplied)
        Result = cBlendPreMultiplied;
    else if (type == gpu_blend_state::additive)
        Result = cBlendAdditive;
    else if (type == gpu_blend_state::tradition_additive)
        Result = cBlendTraditionalAdditive;
    
    return Result;
}

constexpr D3D12_PRIMITIVE_TOPOLOGY_TYPE 
ToD3DTopologyType(gpu_topology top)
{
    D3D12_PRIMITIVE_TOPOLOGY_TYPE result = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
    if (top == gpu_topology::point)
        result = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    else if (top == gpu_topology::line)
        result = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    else if (top == gpu_topology::triangle)
        result = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    else if (top == gpu_topology::patch)
        result = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
    return result;
}

gpu_pso::gpu_pso(gpu_device& Device, gpu_pso_info& Info)
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC Desc{};
    Desc.pRootSignature    = Info.mRootSignature.AsHandle();
    Desc.SampleMask        = UINT_MAX;
    Desc.DepthStencilState = Info.mDepth;
    Desc.RasterizerState   = Info.mRasterizer;
    Desc.BlendState        = Info.mBlend;
    Desc.VS                = Info.mVertexShader;
    Desc.PS                = Info.mPixelShader;
    
    // Set the remaining blend states to the one specified in Info
    for (UINT i = 1; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
        Desc.BlendState.RenderTarget[i] = Desc.BlendState.RenderTarget[0];

    assert(Info.mInputElements.Length() == 0 && "Not handling input elements for a PSO");
    Desc.InputLayout = { nullptr, 0 };

    Desc.DSVFormat             = Info.mDepthFormat;
    Desc.PrimitiveTopologyType = ToD3DTopologyType(Info.mTopology);
    Desc.SampleDesc.Count      = Info.mSampleDesc.mCount;
    Desc.SampleDesc.Quality    = Info.mSampleDesc.mQuality;

    Desc.NumRenderTargets = Info.mRenderTargetFormats.NumRenderTargets;
    for (u32 i = 0; i < Desc.NumRenderTargets; ++i)
        Desc.RTVFormats[i] = Info.mRenderTargetFormats.RTFormats[i];

    AssertHr(Device.AsHandle()->CreateGraphicsPipelineState(&Desc, ComCast(&mHandle)));
}
