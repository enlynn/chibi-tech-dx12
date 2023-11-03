#pragma once

#include <types.h>

#include "gfx_pso.h"
#include "gfx_root_signature.h"
#include "gfx_render_target.h"

class renderpass 
{
public:
    renderpass() = default;
    renderpass(const gfx_pso_info& PsoInfo, const gfx_root_signature& RootSignature);


private:
    gfx_pso            mPso           = {};
    gfx_root_signature mRootSignature = {};
    gfx_render_target  mRenderTarget  = {};
};

// Example Implementation of a renderpass
class draw_quad : public renderpass
{
    draw_quad() = default;
};