#pragma once

#include <types.h>

#include "d3d12_common.h"

class gfx_resource;

enum class attachmen_point : u8
{
    color0,
    color1,
    color2,
    color3,
    color4,
    color5,
    color6,
    color7,
    depth_stencil,
    count,
};

class gfx_render_target
{
public:
    gfx_render_target() = default;

    void AttachTexture(attachmen_point Slot, gfx_resource* Texture);
    void DetachTexture(attachmen_point Slot);

    // Resets the state of the render target (all attached textures are removed)
    void Reset(); 

    // Clear all textures using the clear color.
    void Clear(D3D12_CLEAR_VALUE ClearValue);

    // Resize a RenderTarget based on the width/height. This is done to keep all 
    // attached textures the same dimensions.
    void Resize(u32 Width, u32 Height);

    const gfx_resource* GetTexture(attachmen_point Slot);

    // Returns a packed array of the image formats for the textures assigned to this
    // Render Target. Particularly useful for creating PSOs.
    //
    // TODO(enlynn): Determine if PSO require the RT formats to be packed or sparse.
    D3D12_RT_FORMAT_ARRAY GetAttachmentFormats();
    // Gets the depth format for this RenderTarget. If there is not a Depth target
    // attached, then DXGI_FORMAT_UNKNOWN is returned.
    DXGI_FORMAT           GetDepthFormat();

    // Get the viewport for this render target
    // Scale & bias parameters can be used to specify a split screen viewport
    // (bias parameter is normalized in the range [0...1]). 
    //
    // By default, fullscreen is return.
    //D3D12_VIEWPORT GetViewport(f32 Scale[2] = {1.0f, 1.0f}, v2 Bias = {0.0f, 0.0f},
    //                           f32 MinDepth = 0.0f, f32 MaxDepth = 1.0f);

private:
    // TODO(enlynn): Ideally, we would not be storing the Resource directly, but instead
    // be using a texture_id or something equivalent. This is just an implace solution'
    // until textures are implemented.
    gfx_resource* mAttachments[int(attachmen_point::count)] = {};
    // The expected width and height of the attached textures
    u32           mWidth;
    u32           mHeight;
};