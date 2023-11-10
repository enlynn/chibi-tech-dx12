#include "gpu_resource_state.h"
#include "gpu_command_list.h"

#include <platform/platform.h>

fn_internal gpu_resource_state_map_entry*
GetResourceMapEntry(gpu_resource_state_map& ResourceMap, ID3D12Resource* Handle)
{
    ForRange (int, i, ResourceMap.Length())
    {
        gpu_resource_state_map_entry* Entry = &ResourceMap[i];
        if (Entry->mResourceHandle == Handle) return Entry;
    }

    return nullptr;
}

fn_internal void
RemoveResourceMapEntry(gpu_resource_state_map& ResourceMap, ID3D12Resource* Handle)
{
    ForRange (int, i, ResourceMap.Length())
    {
        gpu_resource_state_map_entry* Entry = &ResourceMap[i];
        // Order is not preserved in the map, so a Swap-Delete is fine.
        if (Entry->mResourceHandle == Handle)
            ResourceMap.RemoveAndSwap(i);
    }
}

gpu_resource_state::gpu_resource_state(D3D12_RESOURCE_STATES State)
{
    mState             = State;
    mSubresourcesCount = 0;
}

void gpu_resource_state::SetSubresourceState(u32 Subresource, D3D12_RESOURCE_STATES State)
{
    // If we are transitioning all resources, then no need to track a subresource
    if (Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
    {
        mState             = State;
        mSubresourcesCount = 0;
        return;
    }

    // Updating a single resource, let's see if it is already being tracked.
    ForRange(u32, i, mSubresourcesCount)
    {
        if (Subresource == mSubresources[i].mIndex)
        {
            mSubresources[i].mState = State;
            return;
        }
    }

    // Subresource is not found, let's insert it.
    assert(mSubresourcesCount + 1 < cMaxSubresources);

    gpu_subresource_state NewSubresource = {};
    NewSubresource.mIndex = Subresource;
    NewSubresource.mState = State;

    mSubresources[mSubresourcesCount] = NewSubresource;
    mSubresourcesCount += 1;
}

D3D12_RESOURCE_STATES gpu_resource_state::GetSubresourceState(u32 Subresource)
{
    D3D12_RESOURCE_STATES Result = mState;

    ForRange(u32, i, mSubresourcesCount)
    {
        if (Subresource == mSubresources[i].mIndex)
        {
            Result = mSubresources[i].mState;
        }
    }

    return Result;
}

gpu_resource_state_tracker::gpu_resource_state_tracker(const allocator& Allocator)
{
    mAllocator               = Allocator.Clone();
    mPendingResourceBarriers = darray<D3D12_RESOURCE_BARRIER>(mAllocator, 10);
    mResourceBarriers        = darray<D3D12_RESOURCE_BARRIER>(mAllocator, 10);
    mFinalResourceState      = gpu_resource_state_map(mAllocator, 10);
}

void gpu_resource_state_tracker::Deinit()
{
    mPendingResourceBarriers.Clear();
    mResourceBarriers.Clear();
    mFinalResourceState.Clear();
    mAllocator = {};
}

// Push a resource barrier
void gpu_resource_state_tracker::ResourceBarrier(D3D12_RESOURCE_BARRIER& Barrier)
{
    if (Barrier.Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION)
    { // Transition type barrier
        D3D12_RESOURCE_TRANSITION_BARRIER& TransitionBarrier = Barrier.Transition;

        // Is this a known Barrier, if so, we know the last used state.
        gpu_resource_state_map_entry* KnownResource = GetResourceMapEntry(mFinalResourceState, TransitionBarrier.pResource);
        if (KnownResource)
        {
            // If this is an updated state and ALL_SUBRESOURCES, then transition all known subresources.
            if (TransitionBarrier.Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES && KnownResource->mState.mSubresourcesCount > 0)
            {
                ForRange (u32, i, KnownResource->mState.mSubresourcesCount)
                {
                    gpu_subresource_state& SubresourceState = KnownResource->mState.mSubresources[i];
                    if (SubresourceState.mState != TransitionBarrier.StateAfter)
                    {
                        D3D12_RESOURCE_BARRIER NewBarrier = Barrier;
                        NewBarrier.Transition.Subresource = SubresourceState.mIndex;
                        NewBarrier.Transition.StateBefore = SubresourceState.mState;
                        mResourceBarriers.PushBack(NewBarrier);
                    }
                }
            }
            else
            { // Transitioning a specific subresource, add a barrier if it's final state is changing
                D3D12_RESOURCE_STATES BeforeState = KnownResource->mState.GetSubresourceState(TransitionBarrier.Subresource);
                if (BeforeState != TransitionBarrier.StateAfter)
                {
                    D3D12_RESOURCE_BARRIER NewBarrier = Barrier;
                    NewBarrier.Transition.StateBefore = BeforeState;
                    mResourceBarriers.PushBack(NewBarrier);
                }
            }

            // Update the Resource State
            KnownResource->mState.SetSubresourceState(TransitionBarrier.Subresource, TransitionBarrier.StateAfter);
        }
        else
        {
            // This is an unknown barrier, add the barrier to a pending List.
            // This list will be resolved before the command list is executed on the GPU.
            mPendingResourceBarriers.PushBack(Barrier);

            // The state of the barrier is now "known" at execution. Can add it to our Know Resource states.
            gpu_resource_state State = {};
            State.SetSubresourceState(TransitionBarrier.Subresource, TransitionBarrier.StateAfter);

            gpu_resource_state_map_entry Entry = { .mResourceHandle = TransitionBarrier.pResource, .mState = State };
            mFinalResourceState.PushBack(Entry);
        }
    }
    else
    { // Either UAV or Aliasing barrier, just add it to the list
        mResourceBarriers.PushBack(Barrier);
    }
}

// Push a transition resource barier
void gpu_resource_state_tracker::TransitionBarrier(
    const gpu_resource*   Resource,
    D3D12_RESOURCE_STATES StateAfter,
    UINT                  SubResource)
{
    if (Resource)
    {
        D3D12_RESOURCE_BARRIER Barrier = {};
        Barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        Barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        Barrier.Transition.pResource   = Resource->AsHandle();
        Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
        Barrier.Transition.StateAfter  = StateAfter;
        Barrier.Transition.Subresource = SubResource;
        ResourceBarrier(Barrier);
    }
}

// Push a UAV Barrier for a given resource
// @param resource: if null, then any UAV access could require the barrier
void gpu_resource_state_tracker::UAVBarrier(const gpu_resource* Resource)
{
    D3D12_RESOURCE_BARRIER Barrier = {};
    Barrier.Type          = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    Barrier.UAV.pResource = Resource->AsHandle();
    ResourceBarrier(Barrier);
}

// Push an aliasing barrier for a given resource.
// @param: both can be null, which indicates that any placed/reserved resource could cause aliasing
void
gpu_resource_state_tracker::AliasBarrier(const gpu_resource* ResourceBefore, const gpu_resource* ResourceAfter)
{
    D3D12_RESOURCE_BARRIER Barrier = {};
    Barrier.Type                     = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
    Barrier.Aliasing.pResourceBefore = ResourceBefore->AsHandle();
    Barrier.Aliasing.pResourceAfter  = ResourceAfter->AsHandle();
    ResourceBarrier(Barrier);
}

// Flush any (non-pending) resource barriers that have been pushed to the resource state tracker.
void gpu_resource_state_tracker::FlushResourceBarriers(gpu_command_list* CommandList)
{
    UINT NumBarriers = (UINT)mResourceBarriers.Length();
    if (NumBarriers > 0)
    {
        CommandList->AsHandle()->ResourceBarrier(NumBarriers, mResourceBarriers.Ptr());
        mResourceBarriers.Reset();
    }
}

// Reset resource state tracking. Done when command list is reset.
void gpu_resource_state_tracker::Reset()
{
    assert(mPendingResourceBarriers.Length() == 0); // This should be handled before submitting the command list.
    assert(mResourceBarriers.Length() == 0);        // Extra barriers submitted that we didn't have to
    mFinalResourceState.Reset();                    // There is no known resource states anymore.
}

void gpu_global_resource_state::SubmitResourceStates(gpu_resource_state_tracker& StateTracker)
{
    const gpu_resource_state_map& FinalResourceStates = StateTracker.GetFinalResourceState();
    for (const auto& State : FinalResourceStates)
    {
        gpu_resource_state_map_entry* KnownResource = GetResourceMapEntry(mKnownStates, State.mResourceHandle);
        if (KnownResource)
            *KnownResource = State;
        else
        {
            gpu_resource_state_map_entry NewState = State;
            mKnownStates.PushBack(NewState);
        }
    }
}

void gpu_global_resource_state::AddResource(gpu_resource& Resource, D3D12_RESOURCE_STATES InitialState, UINT SubResource)
{
    gpu_resource_state_map_entry* KnownResource = GetResourceMapEntry(mKnownStates, Resource.AsHandle());
    if (KnownResource)
    {
        // This is a known resource, we can't override the existing state
        if (SubResource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
        {
            LogWarn("Attempting to add a resource to the global state map, but resource exists. Replacing old resource");
            return;
        }

        // Is this one of the subresources? If so, can't override the existing subresource
        ForRange(u32, i, KnownResource->mState.mSubresourcesCount)
        {
            gpu_subresource_state& SubresourceState = KnownResource->mState.mSubresources[i];
            if (SubresourceState.mIndex == SubResource)
            {
                LogWarn("Attempting to add a resource to the global state map, but resource exists. Replacing old resource");
                return;
            }
        }

        // Add the subresource, but not if it exceeds the allowed amount of subresources
        KnownResource->mState.SetSubresourceState(SubResource, InitialState);
    }
    else
    {
        gpu_resource_state NewState = {};
        NewState.SetSubresourceState(SubResource, InitialState);

        gpu_resource_state_map_entry NewEntry = {};
        NewEntry.mState          = NewState;
        NewEntry.mResourceHandle = Resource.AsHandle();

        mKnownStates.PushBack(NewEntry);
    }
}

void gpu_global_resource_state::RemoveResource(const gpu_resource& Resource)
{
    RemoveResourceMapEntry(mKnownStates, Resource.AsHandle());
}

// Flush any pending resource barriers to the command list
u32 gpu_global_resource_state::FlushPendingResourceBarriers(gpu_command_list& CommandList, gpu_resource_state_tracker& StateTracker)
{
    const darray<D3D12_RESOURCE_BARRIER>& PendingBarriers = StateTracker.GetPendingBarriers();
    darray<D3D12_RESOURCE_BARRIER> BarriersToSubmit = darray<D3D12_RESOURCE_BARRIER>(mKnownStates.GetAllocator().Clone(), PendingBarriers.Length());

    for (const auto& Barrier : PendingBarriers)
    {
        assert(Barrier.Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION);

        const D3D12_RESOURCE_TRANSITION_BARRIER& TransitionBarrier = Barrier.Transition;

        // Is this a known Barrier, if so, we know the last used state.
        gpu_resource_state_map_entry* KnownResource = GetResourceMapEntry(mKnownStates, TransitionBarrier.pResource);
        if (KnownResource)
        { // If this is an updated state and ALL_SUBRESOURCES, then transition all known subresources.
            if (TransitionBarrier.Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
            {
                if (KnownResource->mState.mSubresourcesCount == 0)
                {
                    if (KnownResource->mState.mState != TransitionBarrier.StateAfter)
                    {
                        D3D12_RESOURCE_BARRIER NewBarrier = Barrier;
                        NewBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                        NewBarrier.Transition.StateBefore = KnownResource->mState.mState;
                        BarriersToSubmit.PushBack(NewBarrier);
                    }
                }
                else
                {
                    ForRange(u32, i, KnownResource->mState.mSubresourcesCount)
                    {
                        gpu_subresource_state& SubresourceState = KnownResource->mState.mSubresources[i];
                        if (SubresourceState.mState != TransitionBarrier.StateAfter)
                        {
                            D3D12_RESOURCE_BARRIER NewBarrier = Barrier;
                            NewBarrier.Transition.Subresource = SubresourceState.mIndex;
                            NewBarrier.Transition.StateBefore = SubresourceState.mState;
                            BarriersToSubmit.PushBack(NewBarrier);
                        }
                    }
                }
            }
            else
            { // Transitioning a specific subresource, add a barrier if it's final state is changing
                D3D12_RESOURCE_STATES BeforeState =
                    KnownResource->mState.GetSubresourceState(TransitionBarrier.Subresource);
                if (BeforeState != TransitionBarrier.StateAfter)
                {
                    D3D12_RESOURCE_BARRIER NewBarrier = Barrier;
                    NewBarrier.Transition.StateBefore = BeforeState;
                    BarriersToSubmit.PushBack(NewBarrier);
                }
            }

            // Update the Resource State
            //KnownResource->mState.SetSubresourceState(TransitionBarrier.Subresource, TransitionBarrier.StateAfter);
        }
        else
        {
            // This is an unknown barrier, add the barrier to a pending List.
            // This list will be resolved before the command list is executed on the GPU.
            D3D12_RESOURCE_BARRIER NewBarrier = Barrier;
            BarriersToSubmit.PushBack(NewBarrier);

            // The state of the barrier is now "known" at execution. Can add it to our Know Resource states.
            gpu_resource_state State = {};
            State.SetSubresourceState(TransitionBarrier.Subresource, TransitionBarrier.StateAfter);

            gpu_resource_state_map_entry Entry = { .mResourceHandle = TransitionBarrier.pResource, .mState = State };
            mKnownStates.PushBack(Entry);
        }
    }

    // Submit the barriers.
    UINT NumBarriers = (UINT)BarriersToSubmit.Length();
    if (NumBarriers > 0)
    {
        CommandList.AsHandle()->ResourceBarrier(NumBarriers, BarriersToSubmit);
    }

    // No more pending barriers, so clear the list.
    StateTracker.ClearPendingBarriers();

    return NumBarriers;
}