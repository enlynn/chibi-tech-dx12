#pragma once

#include <types.h>
#include "d3d12_common.h"

#include <util/array.h>
#include <util/str8.h>

class gfx_device;

/*

A Root Signature describes the bindings for a Pipeline.

The Root Signature has space for 64 DWORDs (DWORD = 4 bytes)

Root Constants
- 1 DWORD, for a maximum of 64 Root Constants. For example, you can embed 64 ints into the root signature.
- No indirection
- Inlined directly into the root signature
- Appears as a Constant Buffer in the shaders
- Declared with D3D12_ROOT_CONSTANTS
- Relative root signature type needs to be D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS 

Root Descriptors
- 2 DWORDS (size of a GPU virtual address), for a maximum of 32 descriptors
- 1 layer of indirection
- Inlined in the root arguments
- Made for descriptors that get accessed the most often
- CBV, SRV, UAVs
- Supported SRV/UAV formats are only 32-bit FLOAT/UINT/SINT.
- SRVs of ray tracing acceleration structures are also supported.
- D3D12_ROOT_DESCRIPTOR
- Slot type needs to be D3D12_ROOT_PARAMETER_TYPE_CBV, D3D12_ROOT_PARAMETER_TYPE_SRV, D3D12_ROOT_PARAMETER_TYPE_UAV

Root Descriptor Tables
- 1 DWORD, for a maximum of 64 tables
- 2 layers of indirection
- Pointers to an array of descriptors

Actually creating these...
D3D12_ROOT_PARAMETER 
    D3D12_ROOT_DESCRIPTOR_TABLE 
        Descriptor Ranges
            Range Type
            Num Descriptors
            Base Shader Register
            Register Space
            Offset in Descriptors from Table Start
    D3D12_ROOT_CONSTANTS
        Shader Register
        Register Space
        Num 32 bit values
    D3D12_ROOT_DESCRIPTOR
        Shader Register
        Register Space

*/

enum class gfx_descriptor_type : u8
{
    srv,
    uav,
    cbv,
    sampler,
};

enum class gfx_descriptor_range_flags : u8
{
    none,                // Default behavior, SRV/CBV are static at execute, UAV are volatile 
    constant,            // Data is Constant and Descriptors are Constant. Best for driver optimization.
    dynamic,             // Data is Volatile and Descriptors are Volatile
    data_constant,       // Data is Constant and Descriptors are Volatile
    descriptor_constant, // Data is Volatile and Descriptors are constant
};

struct gfx_root_descriptor
{
    u32                        mRootIndex;                                         // Root Parameter Index must be set
    gfx_descriptor_type        mType           = gfx_descriptor_type::cbv;
    gfx_descriptor_range_flags mFlags          = gfx_descriptor_range_flags::none; // Volatile Descriptors are disallowed for Root Descriptors
    u32                        mShaderRegister = 0;
    u32                        mRegisterSpace  = 0;

    static constexpr u8 cDWordCount = 2;
};

struct gfx_root_constant
{
    u32 mRootIndex;          // Root Parameter Index must be set
    u32 mShaderRegister = 0;
    u32 mRegisterSpace  = 0;
    u32 mNum32bitValues = 1;

    static constexpr u8 cDWordCount = 1;
};

struct gfx_descriptor_range
{
    gfx_descriptor_type        mType               = gfx_descriptor_type::cbv;
    u32                        mNumDescriptors     = 1;
    u32                        mBaseShaderRegister = 0;
    u32                        mRegisterSpace      = 0;
    u32                        mDescriptorOffset   = 0;
    gfx_descriptor_range_flags mFlags              = gfx_descriptor_range_flags::none;
};

struct gfx_descriptor_table
{
    u32                          mRootIndex;            // Root Parameter Index must be set
    farray<gfx_descriptor_range> mDescriptorRanges = {};

    static constexpr u8 cDWordCount = 1;
};

struct gfx_static_sampler_desc
{
    u32                        ShaderRegister   = 0;
    u32                        RegisterSpace    = 0;
    D3D12_FILTER               Filter           = D3D12_FILTER_ANISOTROPIC;
    D3D12_TEXTURE_ADDRESS_MODE AddressU         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    D3D12_TEXTURE_ADDRESS_MODE AddressV         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    D3D12_TEXTURE_ADDRESS_MODE AddressW         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    f32                        MipLODBias       = 0;
    u32                        MaxAnisotropy    = 16;
    D3D12_COMPARISON_FUNC      ComparisonFunc   = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    D3D12_STATIC_BORDER_COLOR  BorderColor      = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    f32                        MinLOD           = 0.0f;
    f32                        MaxLOD           = D3D12_FLOAT32_MAX;
    D3D12_SHADER_VISIBILITY    ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
};

struct gfx_root_signature_info
{
    farray<gfx_descriptor_table>    DescriptorTables    = {};
    farray<gfx_root_descriptor>     Descriptors         = {};
    farray<gfx_root_constant>       DescriptorConstants = {};
    farray<gfx_static_sampler_desc> StaticSamplers      = {};
    // TODO: Static samplers
    istr8                           Name                = {}; // Optional, Set only in debug mode only
};

class gfx_root_signature
{
public:
	gfx_root_signature() = default;
    gfx_root_signature(const gfx_device& Device, const gfx_root_signature_info& Info);

    u32 GetDescriptorTableBitmask(gfx_descriptor_type HeapType) const;
    u32 GetNumDescriptors(u32 RootIndex)                        const;
    u32 GetRootParameterCount()                                 const { return mRootParameterCount; }

    struct ID3D12RootSignature* AsHandle() const { return mHandle; }

    void Release() { if (mHandle) ComSafeRelease(mHandle); }

private:
	struct ID3D12RootSignature* mHandle                     = 0;
    // Total number of root parameters in the root signature
    u32                         mRootParameterCount         = 0;
    // Need to know number of descriptors per table
    //  A maximum of 64 descriptor tables are supported
    u32                         mNumDescriptorsPerTable[64] = {};
    // A bitmask that represents the root parameter indices that 
    // are descriptor tables for Samplers
    u32                         mSamplerTableBitmask        = 0;
    // A bitmask that represents the root parameter indices
    // that are CBV/UAV/SRV descriptor tables
    u32                         mDescriptorTableBitmask     = 0;

    // RootSignature is limited by the number of "bindings" it can have. The upper
    // bound is 64 DWords (64 * 4 bytes).
    static constexpr u8 cMaxDWordCount = 64;
};
