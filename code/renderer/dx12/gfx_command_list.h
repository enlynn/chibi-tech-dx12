#pragma once

#include <types.h>

#include "d3d12/d3d12.h"
#include "d3d12_common.h"
#include "gfx_device.h"
#include "gfx_resource.h"

#include <util/array.h>

class gfx_device;
class gfx_resource;

enum class gfx_command_list_type : u8
{
	none,     //error state
	graphics, //standard graphics command list
	compute,  //standard compute command list
	copy,     //standard copy command list
	indirect, //indirect command list
	count,
};

struct gfx_transition_barrier
{
	D3D12_RESOURCE_STATES BeforeState  = D3D12_RESOURCE_STATE_COMMON;
	D3D12_RESOURCE_STATES AfterState   = D3D12_RESOURCE_STATE_COMMON;
	// Default value is to transition all subresources.
	u32                   Subresources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
};

class gfx_command_list
{
public:
	gfx_command_list() = default;
	explicit gfx_command_list(const gfx_device& Device, gfx_command_list_type Type);

	~gfx_command_list() { Release(); } // NOTE: I bet this will cause problems
	void Release();

	inline gfx_command_list_type             GetType()  const { return mType;   }
	inline struct ID3D12GraphicsCommandList* AsHandle() const { return mHandle; }

	void Reset();
	
	void Close();

	// Texture Handling
	void ClearTexture(const gfx_resource& TextureResource, f32 ClearColor[4]);

	// For now, assume all subresources should be 
	void TransitionBarrier(const gfx_resource& Resource, const gfx_transition_barrier& Barrier);

	// Requires GetCopyableFootprints to be called before passing paramters to this function
	u64 UpdateSubresources(
		const gfx_resource&                        DestinationResource,
		const gfx_resource&                        IntermediateResource,
		u32                                        FirstSubresource,   // Range = (0,D3D12_REQ_SUBRESOURCES)
		u32                                        NumSubresources,    // Range = (0,D3D12_REQ_SUBRESOURCES-FirstSubresource)
		u64                                        RequiredSize,
		farray<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> SubresourceLayouts, // Size == NumSubresources
		farray<u32>                                NumRows,            // Size == NumSubresources
		farray<u64>                                NumRowSizesInBytes, // Size == NumSubresources
		farray<D3D12_SUBRESOURCE_DATA>             SubresourceData     // Size == NumSubresources
	);

	// NOTE(enlynn):
	// I am unsure about these template variations. I feel like in most situations
	// the MaxSubresources will not be known at compile time. Will keep this version 
	// until this is no longer true.

	// The following two variations of UpdateSubresources will set up the D3D12_PLACED_SUBRESOURCE_FOOTPRINT
	// Array, but requires a set of arrays to be initialized based on the number of resources for the buffer.
	// These two functions stack allocate these arrays, so avoid using D3D12_REQ_SUBRESOURCES as the upper bound.
	template <u32 MaxSubresources>
    u64 UpdateSubresources(
		const gfx_resource&            DestinationResource,
		const gfx_resource&            IntermediateResource,
		u64                            IntermediateOffset,
		u32                            FirstSubresource,   // Range (0, MaxSubresources)
		u32                            NumSubresources,    // Range (1, MaxSubresources - FirstSubresource)
		farray<D3D12_SUBRESOURCE_DATA> SourceData          // Range List (Size = NumSubresources)
	) {
        u64                                RequiredSize = 0;
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT Layouts[MaxSubresources];
        UINT                               NumRows[MaxSubresources];
        UINT64                             RowSizesInBytes[MaxSubresources];
        
        D3D12_RESOURCE_DESC Desc = DestinationResource.GetResourceDesc();
        mDevice->AsHandle()->GetCopyableFootprints(&Desc, FirstSubresource, NumSubresources, IntermediateOffset, Layouts, NumRows, RowSizesInBytes, &RequiredSize);
        
        return UpdateSubresources(
			DestinationResource, IntermediateResource, FirstSubresource, 
			NumSubresources, RequiredSize, Layouts, NumRows, RowSizesInBytes, SourceData);
    }


	void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type, ID3D12DescriptorHeap* Heap);
	void BindDescriptorHeaps();

private:
	gfx_command_list_type             mType                                                       = gfx_command_list_type::none;
	struct ID3D12GraphicsCommandList* mHandle                                                     = nullptr;
	struct ID3D12CommandAllocator*    mAllocator                                                  = nullptr;

	const gfx_device*                 mDevice                                                     = nullptr;

	ID3D12DescriptorHeap*             mBoundDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {};
};