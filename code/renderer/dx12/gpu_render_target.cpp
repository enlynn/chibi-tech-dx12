#include "gpu_render_target.h"
#include "gpu_resource.h"

#include <platform/platform.h>

void gpu_render_target::AttachTexture(attachment_point Slot, gpu_texture* Texture)
{
#if defined(DEBUG_BUILD)
    if (mAttachments[u32(Slot)] != nullptr)
    {
        LogWarn("Attaching Texture to Slot (%d), but texture already exists. Replacing old texture.", u32(Slot));
    }

    D3D12_RESOURCE_DESC TexDesc = Texture->GetResourceDesc();
    if (TexDesc.Width != mWidth || TexDesc.Height != mHeight)
    {
        LogWarn("Attaching Texture to RenderTarget (), Texture dimensions do not match. Render Target Dim (%d, %d), Texture Dims (%d, %d).",
            mWidth, mHeight, TexDesc.Width, TexDesc.Height);

        mWidth  = TexDesc.Width;
        mHeight = TexDesc.Height;
    }
#endif

    mAttachments[u32(Slot)] = Texture;
}

void gpu_render_target::DetachTexture(attachment_point Slot)
{
    mAttachments[u32(Slot)] = nullptr;
}

// Resets the state of the render target (all attached textures are removed)
void gpu_render_target::Reset()
{
    ForRange(int, i, int(attachment_point::count))
    {
        mAttachments[i] = nullptr;
    }
}

D3D12_VIEWPORT
gpu_render_target::GetViewport(f32x2 Scale, f32x2 Bias, f32 MinDepth, f32 MaxDepth)
{
    u32 Width  = 1;
    u32 Height = 1;

    ForRange(int, i, int(attachment_point::depth_stencil))
    {
        if (mAttachments[i])
        {
            D3D12_RESOURCE_DESC Desc = mAttachments[i]->GetResourceDesc();
            Width                    = FastMax(Width, Desc.Width);
            Height                   = FastMax(Height, Desc.Height);
        }
    }

    D3D12_VIEWPORT Viewport = {
        .TopLeftX = Width  * Bias.X,
        .TopLeftY = Height * Bias.Y,
        .Width    = Width  * Scale.X,
        .Height   = Height * Scale.Y,
        .MinDepth = MinDepth,
        .MaxDepth = MaxDepth,
    };

    return Viewport;
}

D3D12_RT_FORMAT_ARRAY
gpu_render_target::GetAttachmentFormats()
{
    D3D12_RT_FORMAT_ARRAY Formats = {};

    ForRange(int, i, int(attachment_point::depth_stencil))
    {
        D3D12_RESOURCE_DESC Desc = mAttachments[i]->GetResourceDesc();

        Formats.RTFormats[Formats.NumRenderTargets] = Desc.Format;
        Formats.NumRenderTargets += 1;
    }

    return Formats;
}

const gpu_texture*
gpu_render_target::GetTexture(attachment_point Slot)
{
    return mAttachments[int(Slot)];
}

DXGI_FORMAT gpu_render_target::GetDepthFormat()
{
    DXGI_FORMAT Result = DXGI_FORMAT_UNKNOWN;

    const gpu_texture* DepthTexture = mAttachments[int(attachment_point::depth_stencil)];
    if (DepthTexture != nullptr)
    {
        D3D12_RESOURCE_DESC Desc = DepthTexture->GetResourceDesc();
        Result = Desc.Format;
    }
    return Result;
}

void gpu_render_target::Bind(struct gpu_command_list* CommandList,
                             D3D12_CLEAR_VALUE*       ClearValue,
                             bool                     ClearDepthStencil)
{
#if 0
gpu_transition_barrier ToClearBarrier = {};
	ToClearBarrier.BeforeState = D3D12_RESOURCE_STATE_COMMON;
	ToClearBarrier.AfterState  = D3D12_RESOURCE_STATE_RENDER_TARGET;
	List->TransitionBarrier(Backbuffer, ToClearBarrier);

	f32 ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	List->AsHandle()->ClearRenderTargetView(SwapchainDescriptor.GetDescriptorHandle(), ClearColor, 0, nullptr);

	// Set the render target (hardcode for now)
	D3D12_CPU_DESCRIPTOR_HANDLE RtHandles[] = { SwapchainDescriptor.GetDescriptorHandle() };
	List->AsHandle()->OMSetRenderTargets(1, RtHandles, FALSE, nullptr);
#endif

    // Transition all textures to Render Target
    ForRange(int, i, int(attachment_point::depth_stencil))
    {

    }

    // Clear Textures if a Clear Value was provided


    // Clear Depth-Stencil if asked for and has a valid Depth Texture


    // Collect the textures and bind the render target.
    D3D12_CPU_DESCRIPTOR_HANDLE RTHandles[int(attachment_point::count)];
    u32 RTHandlesCount = 0;


}
