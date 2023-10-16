#include "gfx_resource.h"
#include "gfx_device.h"
#include "gfx_utils.h"

gfx_resource::gfx_resource(const gfx_device& Device, const D3D12_RESOURCE_DESC& Desc, std::optional <D3D12_CLEAR_VALUE> ClearValue)
	: mClearValue(ClearValue)
{
	D3D12_HEAP_PROPERTIES Properties = GetHeapProperties();
	D3D12_CLEAR_VALUE*    Value      = mClearValue.has_value() ? &mClearValue.value() : nullptr;

	AssertHr(Device.AsHandle()->CreateCommittedResource(&Properties, D3D12_HEAP_FLAG_NONE, &Desc,
		D3D12_RESOURCE_STATE_COMMON, Value, ComCast(&mHandle)));
	// TODO(enlynn): global resource management? -> State: D3D12_RESOURCE_STATE_COMMON

	CheckFeatureSupport(Device);
}

gfx_resource::gfx_resource(const gfx_device& Device, ID3D12Resource* Resource, std::optional <D3D12_CLEAR_VALUE> ClearValue)
	: mClearValue(ClearValue)
	, mHandle(Resource)
{
	CheckFeatureSupport(Device);
}

void 
gfx_resource::Release()
{
	if (IsValid())
	{
		ComSafeRelease(mHandle);
		mClearValue = std::nullopt;
		mFormatSupport = {};
	}
}

void
gfx_resource::CheckFeatureSupport(const gfx_device& Device)
{
	D3D12_RESOURCE_DESC ResourceDesc = mHandle->GetDesc();
	mFormatSupport.Format = ResourceDesc.Format;
	AssertHr(Device.AsHandle()->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &mFormatSupport, sizeof(D3D12_FEATURE_DATA_FORMAT_SUPPORT)));
}

bool
gfx_resource::CheckFormatSupport(D3D12_FORMAT_SUPPORT1 formatSupport)
{
	return (mFormatSupport.Support1 & formatSupport) != 0;
}

bool
gfx_resource::CheckFormatSupport(D3D12_FORMAT_SUPPORT2 formatSupport)
{
	return (mFormatSupport.Support2 & formatSupport) != 0;
}

D3D12_RESOURCE_DESC 
gfx_resource::GetResourceDesc()
{
	D3D12_RESOURCE_DESC Result = {};
	if (IsValid())
	{
		Result = mHandle->GetDesc();
	}
	return Result;
}