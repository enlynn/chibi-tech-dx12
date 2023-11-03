#pragma once

#include <types.h>
#include "d3d12_common.h"

#include <optional>

class gfx_device;

class gfx_resource
{
public:
	gfx_resource() = default;
	// Usually external code will not directly initialize a gfx_resource. Usually it will be a Swapchain or Storage Buffer
	gfx_resource(const gfx_device& Device, const D3D12_RESOURCE_DESC& Desc, std::optional <D3D12_CLEAR_VALUE> ClearValue = std::nullopt);
	// Assumes ownership of the resource
	gfx_resource(const gfx_device& Device, ID3D12Resource* Resource, std::optional <D3D12_CLEAR_VALUE> ClearValue = std::nullopt);

	~gfx_resource() {} // For a gfx_resource, we need to explicitly clear the value to prevent erroneous resource frees
	
	void                                    Release();

	bool                                    CheckFormatSupport(D3D12_FORMAT_SUPPORT1 FormatSupport);
	bool                                    CheckFormatSupport(D3D12_FORMAT_SUPPORT2 FormatSupport);
	// Check the format support and populate the mFormatSupport structure.
	void                                    CheckFeatureSupport(const gfx_device& Device);

	D3D12_RESOURCE_DESC                     GetResourceDesc() const;

	inline ID3D12Resource*                  AsHandle()      const { return mHandle;            }
	inline bool                             IsValid()       const { return mHandle != nullptr; }
	inline std::optional<D3D12_CLEAR_VALUE> GetClearValue() const { return mClearValue;        }

private:
	ID3D12Resource*                   mHandle        = nullptr;
	D3D12_FEATURE_DATA_FORMAT_SUPPORT mFormatSupport = {};
	std::optional<D3D12_CLEAR_VALUE>  mClearValue    = {};
};

#if 0
enum class gfx_resource_view_type
{
	srv, // Shader Resource View
	uav, // Unordered Access View
};

struct gfx_resource_view
{
	// Create a Shader Resource View
	gfx_resource_view(gfx_resource& Resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& SRV);
	// Create a Unordered Access Resource View
	gfx_resource_view(gfx_resource& Resource, gfx_resource& CounterResource, const D3D12_SHADER_RESOURCE_VIEW_DESC& SRV);

	~gfx_resource_view() {} // For a gfx_resource_view, we need to explicitly clear to prevent erroneous resource frees
	void Deinit();



	gfx_resource_view_type mType;
	gfx_resource&          mBorrowedResource;
	gfx_resource&          mBorrowedUAVCounterResource; // Optional, for UAV
};
#endif