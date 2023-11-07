#include "gpu_render_target.h"
#include "gpu_resource.h"

#include <platform/platform.h>

void gpu_render_target::AttachTexture(attachment_point Slot, gpu_resource* Texture)
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

void gpu_render_target::Clear(D3D12_CLEAR_VALUE ClearValue)
{

}
