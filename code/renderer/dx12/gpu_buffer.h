#pragma once

#include <types.h>

#include "d3d12_common.h"

#include "gpu_resource.h"
#include "gpu_descriptor_allocator.h"

class gpu_command_list;

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
    //bool                mCpuVisible         = false;   // If True, then can write to the buffer without needing an intermediate buffer
    bool                mCreateResourceView = false;
    u32                 mStride             = 1;
    u32                 mCount              = 1;       // FrameSize = mStride * mCount
    u32                 mBufferFrames       = 1;       // TotalSize = FrameSize * mBufferFrames
    void*               mData               = nullptr;
    //gpu_resource*       mBufferHandle       = nullptr;
};

class gpu_buffer
{
public:
    gpu_buffer() = default;

    // Helper functions to make specific buffer types.
    //static gpu_buffer MakeVertexBuffer();
    static gpu_buffer CreateIndexBuffer(struct gpu_frame_cache* FrameCache, gpu_index_buffer_info& Info);
    //static gpu_buffer MakeStructuredBuffer();
    static gpu_buffer CreateByteAdressBuffer(struct gpu_frame_cache* FrameCache, gpu_byte_address_buffer_info& Info);

    //void                     CreateViews();

    // Convenience functions for accessing the buffer views
    D3D12_INDEX_BUFFER_VIEW  GetIndexBufferView()       const { return _views.mIndexView;    }
    D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView()      const { return _views.mVertexView;   }
    cpu_descriptor           GetResourceView()          const { return _views.mResourceView; }
    cpu_descriptor           GetConstantBufferView()    const { return GetResourceView();    }
    cpu_descriptor           GetStructuredBufferView()  const { return GetResourceView();    }
    cpu_descriptor           GetByteAddressBufferView() const { return GetResourceView();    }

    u32                       GetIndexCount()  const { return mCount;             }
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress()  const { mResource.GetGPUAddress(); }
    gpu_resource*             GetGPUResource()       { return &mResource;         }

private: 
    static gpu_resource CopyBuffer(
        struct gpu_frame_cache* FrameCache,  
        void*                   BufferData, 
        u64                     BufferSize, 
        D3D12_RESOURCE_FLAGS    Flags              = D3D12_RESOURCE_FLAG_NONE, 
        D3D12_RESOURCE_STATES   InitialBufferState = D3D12_RESOURCE_STATE_COMMON);

    gpu_buffer_type mType         = gpu_buffer_type::unknown;
    gpu_resource    mResource     = {};

    u32             mStride       = 0;
    u32             mCount        = 0;
    u32             mBufferFrames = 0;

    bool            mCpuVisible   = false;
    bool            mIsBound      = false;
    void*           mBoundData    = nullptr;
    // TODO: Intermediary buffer for non-cpu visible buffers

    union
    {
        D3D12_INDEX_BUFFER_VIEW  mIndexView = {};    // Index Buffers
        D3D12_VERTEX_BUFFER_VIEW mVertexView;        // Vertex Buffers
        cpu_descriptor           mResourceView;      // Byte Address, Structured, and Constant Buffers
    } _views;
};
