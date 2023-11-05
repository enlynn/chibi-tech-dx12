#pragma once

#include <types.h>

#include "gpu_resource.h"

//
// Some thoughts:
// - One Buffer to Rule them All
// --- The behavior between them is *mostly* the same.
// - It is not the buffer's responsibility to upload or destroy itself
// --- This is the responsibility of a higher level structure (i.e. mesh_manager)
// 

enum class gpu_buffer_type : u8
{
    unknown,
    vertex,       // TODO: Unsupported
    index,
    structured,   // TODO: Unsupported
    byte_address,
};

// struct gpu_vertex_buffer_info
// {
// };

struct gpu_index_buffer_info
{
    bool          mIsU16        = false;   // If false, then assumed to be U32
    u32           mIndexCount   = 0;
    void*         mIndices      = nullptr;
    gpu_resource* mBufferHandle = nullptr;
};

// struct gpu_structured_buffer_info
// {
// };

struct gpu_byte_address_buffer_info
{
    bool                mCpuVisible         = false;   // If True, then can write to the buffer without needing an intermediate buffer
    bool                mCreateResourceView = false;
    u32                 mStride             = 1;
    u32                 mCount              = 1;       // FrameSize = mStride * mCount
    u32                 mBufferFrames       = 1;       // TotalSize = FrameSize * mBufferFrames
    gpu_resource*       mBufferHandle       = nullptr;
};

class gpu_buffer
{
public:
    // Helper functions to make specific buffer types.
    //static gpu_buffer MakeVertexBuffer();
    static gpu_buffer CreateIndexBuffer();
    //static gpu_buffer MakeStructuredBuffer();
    static gpu_buffer CreateByteAdressBuffer();

    void  CreateViews();

    D3D12_INDEX_BUFFER_VIEW GetIndexBufferView();
    D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView();

    cpu_descriptor GetResourceView()          const;
    cpu_descriptor GetConstantBufferView()    const;
    cpu_descriptor GetStructuredBufferView()  const;
    cpu_descriptor GetByteAddressBufferView() const;

    // If not a per-frame buffer, then FrameIndex can be ignored.
    void* BindBuffer(u32 FrameIndex = 0);
    void  UnbindBuffer();

protected:
    gpu_buffer() = default;

private: 
    gpu_buffer_type mType         = gpu_buffer_type::unknown;
    gpu_resource    mResource     = mResource;

    u32             mStride       = 0;
    u32             mCount        = 0;
    u32             mBufferFrames = 0;

    bool            mCpuVisible   = false;
    bool            mIsBound      = false;
    void*           mBoundData    = nullptr;
    // TODO: Intermediary buffer for non-cpu visible buffers

    union
    {
        D3D12_INDEX_BUFFER_VIEW  mIndexView;    // Index Buffers
        D3D12_VERTEX_BUFFER_VIEW mVertexView;   // Vertex Buffers
        cpu_descriptor           mResourceView; // Byte Address, Structured, and Constant Buffers
    } _views;
};