#include "gfx_command_list.h"
#include "d3d12_common.h"
#include "gfx_device.h"
#include "gfx_resource.h"

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

void gfx_command_list::ClearTexture(gfx_resource& TextureResource, f32 ClearColor[4])
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
