#include "gfx_command_list.h"

#include <platform/platform.h>

inline void 
MemcpySubresource(
	const D3D12_MEMCPY_DEST*      Destination,
	const D3D12_SUBRESOURCE_DATA* Source,
	u64                           RowSizeInBytes,
	u32                           NumRows,
	u32                           NumSlices)
{
	ForRange (UINT, z, NumSlices)
	{
		auto pDestSlice = (BYTE*)(Destination->pData) + Destination->SlicePitch * z;
		auto pSrcSlice = (const BYTE*)(Source->pData) + Source->SlicePitch * LONG_PTR(z);
		for (UINT y = 0; y < NumRows; ++y)
		{
			memcpy(
				pDestSlice + Destination->RowPitch * y,
				pSrcSlice + Source->RowPitch * LONG_PTR(y),
				RowSizeInBytes
			);
		}
	}
}

inline D3D12_TEXTURE_COPY_LOCATION
GetTextureCopyLoction(ID3D12Resource* Resource, u32 SubresourceIndex)
{
	D3D12_TEXTURE_COPY_LOCATION Result = {};
	Result.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	Result.PlacedFootprint  = {};
	Result.SubresourceIndex = SubresourceIndex;
	Result.pResource        = Resource;
	return Result;
}

inline D3D12_TEXTURE_COPY_LOCATION
GetTextureCopyLoction(ID3D12Resource* Resource, const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& Footprint)
{
	D3D12_TEXTURE_COPY_LOCATION Result = {};
	Result.Type            = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	Result.PlacedFootprint = Footprint;
	Result.pResource       = Resource;
	return Result;
}

fn_inline D3D12_COMMAND_LIST_TYPE 
ToD3D12CommandListType(gfx_command_list_type Type)
{
#if 0
D3D12_COMMAND_LIST_TYPE_DIRECT = 0,
D3D12_COMMAND_LIST_TYPE_BUNDLE = 1,
D3D12_COMMAND_LIST_TYPE_COMPUTE = 2,
D3D12_COMMAND_LIST_TYPE_COPY = 3,
D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE = 4,
D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS = 5,
D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE = 6,
D3D12_COMMAND_LIST_TYPE_NONE = -1
#endif

	switch (Type)
	{
		case gfx_command_list_type::graphics: // intentional fallthrough
		case gfx_command_list_type::indirect: return D3D12_COMMAND_LIST_TYPE_DIRECT;
		case gfx_command_list_type::compute:  return D3D12_COMMAND_LIST_TYPE_COMPUTE;
		case gfx_command_list_type::copy:     return D3D12_COMMAND_LIST_TYPE_COPY;
		default:                              return D3D12_COMMAND_LIST_TYPE_NONE;
		
	}
}

gfx_command_list::gfx_command_list(const gfx_device& Device, gfx_command_list_type Type)
	: mType(Type)
	, mDevice(&Device)
{
	const D3D12_COMMAND_LIST_TYPE D3D12Type = ToD3D12CommandListType(Type);

	AssertHr(Device.AsHandle()->CreateCommandAllocator(D3D12Type, ComCast(&mAllocator)));
	AssertHr(Device.AsHandle()->CreateCommandList(0, D3D12Type, mAllocator, nullptr, ComCast(&mHandle)));

	// A command list is created in the recording state, but since the Command Queue is responsible for fetching
	// a command list, it is ok for the list to stay in this state.
}

void gfx_command_list::Release()
{
	if (mAllocator)
		ComSafeRelease(mAllocator);
	if (mHandle)
		ComSafeRelease(mHandle);
}

void gfx_command_list::Reset() 
{
	AssertHr(mAllocator->Reset());
	AssertHr(mHandle->Reset(mAllocator, nullptr));
}

void gfx_command_list::Close()
{
	mHandle->Close();
}

void gfx_command_list::ClearTexture(const gfx_resource& TextureResource, f32 ClearColor[4])
{
	D3D12_RESOURCE_BARRIER Barrier = {};
	Barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	Barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	Barrier.Transition.pResource   = TextureResource.AsHandle();
	Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON; // TODO: I don't think this accurate, but let's see what happens...
	Barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
	Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	// Submit barrier?


	//mHandle->ClearRenderTargetView(, ClearColor, 0, nullptr);
}

void 
gfx_command_list::TransitionBarrier(const gfx_resource& Resource, const gfx_transition_barrier& Barrier)
{
	D3D12_RESOURCE_BARRIER TransitionBarrier = {};
	TransitionBarrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	TransitionBarrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	TransitionBarrier.Transition.pResource   = Resource.AsHandle();
	TransitionBarrier.Transition.StateBefore = Barrier.BeforeState; // TODO: I don't think this accurate, but let's see what happens...
	TransitionBarrier.Transition.StateAfter  = Barrier.AfterState;
	TransitionBarrier.Transition.Subresource = Barrier.Subresources;

	// TODO(enlynn): Ideally I wouldn't immediately flush the barriers, but since I haven't written
	// ResourceState management, just flush the barriers immediately.
	mHandle->ResourceBarrier(1, &TransitionBarrier);
}

u64 
gfx_command_list::UpdateSubresources(
	const gfx_resource&                        DestinationResource,
	const gfx_resource&                        IntermediateResource,
	u32                                        FirstSubresource,   // Range = (0,D3D12_REQ_SUBRESOURCES)
	u32                                        NumSubresources,    // Range = (0,D3D12_REQ_SUBRESOURCES-FirstSubresource)
	u64                                        RequiredSize,
	farray<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> SubresourceLayouts, // Size == NumSubresources
	farray<u32>                                NumRows,            // Size == NumSubresources
	farray<u64>                                NumRowSizesInBytes, // Size == NumSubresources
	farray<D3D12_SUBRESOURCE_DATA>             SubresourceData     // Size == NumSubresources
) {
	// Quick error checking.
	if (FirstSubresource > D3D12_REQ_SUBRESOURCES)
	{
		LogFatal("gfx_command_list::UpdateSubresources : First Subresource should be between (0, 30720), but %d was provided.", FirstSubresource);
	}

	if (NumSubresources > (D3D12_REQ_SUBRESOURCES - FirstSubresource))
	{
		LogFatal("gfx_command_list::UpdateSubresources : Number of Subresources should be between (FirstSubresource, 30720), but %d was provided.", NumSubresources);
	}

	if (SubresourceLayouts.Length() > 0 && SubresourceLayouts.Length() != NumSubresources)
	{
		LogFatal("gfx_command_list::UpdateSubresources : Number of Subresource Layouts should be the number of subresources to update, but was %d.", SubresourceLayouts.Length());
	}

	if (SubresourceLayouts.Length() > 0 && NumRows.Length() != NumSubresources)
	{
		LogFatal("gfx_command_list::UpdateSubresources : Number of Subresource Rows should be the number of subresources to update, but was %d.", NumRows.Length());
	}

	if (SubresourceLayouts.Length() > 0 && NumRowSizesInBytes.Length() != NumSubresources)
	{
		LogFatal("gfx_command_list::UpdateSubresources : Number of Subresource Row Sizes should be the number of subresources to update, but was %d.", NumRowSizesInBytes.Length());
	}

	if (SubresourceLayouts.Length() > 0 && SubresourceData.Length() != NumSubresources)
	{
		LogFatal("gfx_command_list::UpdateSubresources : Number of Subresource Datas should be the number of subresources to update, but was %d.", SubresourceData.Length());
	}

	// Some more minor validation
	D3D12_RESOURCE_DESC IntermediateDesc = IntermediateResource.GetResourceDesc();
	D3D12_RESOURCE_DESC DestinationDesc  = DestinationResource.GetResourceDesc();

	if (IntermediateDesc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER ||
		IntermediateDesc.Width < RequiredSize + SubresourceLayouts[u64(0)].Offset ||
		RequiredSize > u64(-1) ||
		(DestinationDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER &&
			(FirstSubresource != 0 || NumSubresources != 1)))
	{
		return 0;
	}

	ID3D12Resource* pIntermediate = IntermediateResource.AsHandle();
	ID3D12Resource* pDestination  = DestinationResource.AsHandle();

	BYTE* pData;
	HRESULT hr = pIntermediate->Map(0, nullptr, (void**)(&pData));
	if (FAILED(hr))
	{
		return 0;
	}

	ForRange (UINT, i, NumSubresources)
	{
		if (NumRowSizesInBytes[i] > SIZE_T(-1)) 
			return 0;

		D3D12_MEMCPY_DEST DestData = { 
			pData + SubresourceLayouts[i].Offset, 
			SubresourceLayouts[i].Footprint.RowPitch, 
			SIZE_T(SubresourceLayouts[i].Footprint.RowPitch) * SIZE_T(NumRows[i]) 
		};

		MemcpySubresource(
			&DestData, &SubresourceData[i], (SIZE_T)(NumRowSizesInBytes[i]), 
			NumRows[i], SubresourceLayouts[i].Footprint.Depth);
	}
	pIntermediate->Unmap(0, nullptr);

	if (DestinationDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
	{
		u64 SubresourceOffset = SubresourceLayouts[(u64)0].Offset;
		u32 SubresourceWidth  = SubresourceLayouts[(u64)0].Footprint.Width;
		mHandle->CopyBufferRegion(pDestination, 0, pIntermediate, SubresourceOffset, SubresourceWidth);
	}
	else
	{
		ForRange (UINT, i, NumSubresources)
		{
			D3D12_TEXTURE_COPY_LOCATION Dst = GetTextureCopyLoction(pDestination, i + FirstSubresource);
			D3D12_TEXTURE_COPY_LOCATION Src = GetTextureCopyLoction(pIntermediate, SubresourceLayouts[i]);
			mHandle->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
		}
	}
	return RequiredSize;
}

void 
gfx_command_list::SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type, ID3D12DescriptorHeap* Heap)
{
	if (mBoundDescriptorHeaps[Type] != Heap)
	{
		mBoundDescriptorHeaps[Type] = Heap;
		BindDescriptorHeaps();
	}
}

void
gfx_command_list::BindDescriptorHeaps()
{
	UINT                  NumHeaps                                          = 0;
	ID3D12DescriptorHeap* HeapsToBind[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {};

	ForRange(int, i, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES)
	{
		if (mBoundDescriptorHeaps[i] != nullptr)
		{
			HeapsToBind[NumHeaps] = mBoundDescriptorHeaps[i];
			NumHeaps             += 1;
		}
	}

	mHandle->SetDescriptorHeaps(NumHeaps, HeapsToBind);
}