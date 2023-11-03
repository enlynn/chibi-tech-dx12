#pragma once

#include <types.h>

#include "gfx_resource.h"

//
// Some thoughts:
// - One Buffer to Rule them All
// --- The behavior between them is *mostly* the same.
// - It is not the buffer's responsibility to upload or destroy itself
// --- This is the responsibility of a higher level structure (i.e. mesh_manager)
// 

enum class gfx_buffer_type : u8
{
    unknown,
    vertex,       // TODO: Unsupported
    index,
    structured,   // TODO: Unsupported
    byte_address,
};

// struct gfx_vertex_buffer_info
// {
// };

struct gfx_index_buffer_info
{
    bool          mIsU16        = false;   // If false, then assumed to be U32
    u32           mIndexCount   = 0;
    void*         mIndices      = nullptr;
    gfx_resource* mBufferHandle = nullptr;
};

// struct gfx_structured_buffer_info
// {
// };

struct gfx_byte_address_buffer_info
{
    bool                mCpuVisible   = false;   // If True, then can write to the buffer without needing an intermediate buffer
    u32                 mStride       = 1;
    u32                 mCount        = 1;       // FrameSize = mStride * mCount
    u32                 mBufferFrames = 1;       // TotalSize = FrameSize * mBufferFrames
    gfx_resource*       mBufferHandle = nullptr;
};

class gfx_buffer
{
public:
    // Helper functions to make specific buffer types.
    //static gfx_buffer MakeVertexBuffer();
    static gfx_buffer MakeIndexBuffer();
    //static gfx_buffer MakeStructuredBuffer();
    static gfx_buffer MakeByteAdressBuffer();

    gfx_buffer() = default;

    void CreateViews();

    // If not a per-frame buffer, then FrameIndex can be ignored.
    void* BindBuffer(u32 FrameIndex = 0);
    void  UnbindBuffer();

private: 
    gfx_buffer_type mType         = gfx_buffer_type::unknown;
    gfx_resource    mResource     = mResource;

    u32             mStride       = 0;
    u32             mCount        = 0;
    u32             mBufferFrames = 0;

    bool            mCpuVisible   = false;
    bool            mIsBound      = false;
    void*           mBoundData    = nullptr;
    // TODO: Intermediary buffer for non-cpu visible buffers
};