#pragma once

#include <types.h>

#include "gpu_pso.h"
#include "gpu_root_signature.h"
#include "gpu_render_target.h"

class renderpass 
{
public:
    renderpass() = default;
    renderpass(const gpu_pso_info& PsoInfo, const gpu_root_signature& RootSignature);


private:
    gpu_pso            mPso           = {};
    gpu_root_signature mRootSignature = {};
    gpu_render_target  mRenderTarget  = {};
};

// Example Implementation of a renderpass
class draw_quad : public renderpass
{
    draw_quad() = default;
};