
#pragma once

#include "d3d12_common.h"

#include <util/array.h>
#include <util/allocator.h>

struct gpu_subresource_state
{
    u32                   mIndex = 0;
    D3D12_RESOURCE_STATES mState = D3D12_RESOURCE_STATE_COMMON;
};

struct gpu_resource_state
{
    //gpu_resource_state() = default;
    gpu_resource_state(D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON);

    void SetSubresourceState(u32 Subresource, D3D12_RESOURCE_STATES State = D3D12_RESOURCE_STATE_COMMON);
    D3D12_RESOURCE_STATES GetSubresourceState(u32 Subresource);

    D3D12_RESOURCE_STATES mState                          = D3D12_RESOURCE_STATE_COMMON;

    // TODO(enlynn): Not entirely sure what a reasonable upper bound for subresources would be. For now
    // going to just declare 10 and see how it goes. If it much more than this, will likely need a dynamic array.
    static constexpr u32 cMaxSubresources                 = 10;
    gpu_subresource_state mSubresources[cMaxSubresources] = {};
    u32                   mSubresourcesCount              = 0;
};

struct gpu_resource_state_map_entry
{
    ID3D12Resource*    mResourceHandle = nullptr;
    gpu_resource_state mState          = {};
};

using gpu_resource_state_map = darray<gpu_resource_state_map_entry>;

class gpu_resource_state_tracker
{
public:
    gpu_resource_state_tracker() = default;
    gpu_resource_state_tracker(const allocator& Allocator);
    void Deinit();

    // Push a resource barrier
    void ResourceBarrier(D3D12_RESOURCE_BARRIER& Barrier);

    // Push a transition resource barier
    void TransitionBarrier(const class gpu_resource* Resource,
                           D3D12_RESOURCE_STATES     StateAfter,
                           UINT                      SubResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

    // Push a UAV Barrier for a given resource
    // @param resource: if null, then any UAV access could require the barrier
    void UAVBarrier(const class gpu_resource* Resource = 0);

    // Push an aliasing barrier for a given resource.
    // @param: both can be null, which indicates that any placed/reserved resource could cause aliasing
    void AliasBarrier(const class gpu_resource* ResourceBefore = 0, const class gpu_resource* ResourceAfter = 0);

    // Flush any (non-pending) resource barriers that have been pushed to the resource state tracker.
    void FlushResourceBarriers(class gpu_command_list* CommandList);

    // Reset resource state tracking. Done when command list is reset.
    void Reset();

    const darray<D3D12_RESOURCE_BARRIER>& GetPendingBarriers()   const { return mPendingResourceBarriers; }
    void                                  ClearPendingBarriers()       { mPendingResourceBarriers.Reset(); }

    const gpu_resource_state_map& GetFinalResourceState()   const { return mFinalResourceState; }
    void                          ClearFinalResourceState()       { mFinalResourceState.Reset(); }

private:
    allocator                      mAllocator;
    darray<D3D12_RESOURCE_BARRIER> mPendingResourceBarriers = {};
    darray<D3D12_RESOURCE_BARRIER> mResourceBarriers        = {};
    gpu_resource_state_map         mFinalResourceState      = {};
};

struct gpu_global_resource_state
{
    gpu_resource_state_map mKnownStates;

    // Submit known resource state to the global tracker.
    void SubmitResourceStates(gpu_resource_state_tracker& StateTracker);
    // Flush any pending resource barriers to the command list
    u32 FlushPendingResourceBarriers(class gpu_command_list& CommandList, gpu_resource_state_tracker& StateTracker);

    void AddResource(class gpu_resource& Resource, D3D12_RESOURCE_STATES InitialState, UINT SubResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
    void RemoveResource(const class gpu_resource& Resource);
};

void ResourceStateTrackerInit(const allocator& Allocator);
void ResourceStateTrackerDeinit();