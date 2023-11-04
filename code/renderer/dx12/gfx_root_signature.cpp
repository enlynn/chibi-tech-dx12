#include "gfx_root_signature.h"
#include "gfx_device.h"

#include <platform/platform.h>

inline D3D12_DESCRIPTOR_RANGE_TYPE 
GfxDescriptorTypeToD3D12(gfx_descriptor_type Type)
{
    switch (Type)
    {
        case gfx_descriptor_type::cbv:     return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        case gfx_descriptor_type::sampler: return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
        case gfx_descriptor_type::srv:     return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        case gfx_descriptor_type::uav:     return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    }
}

inline D3D12_ROOT_PARAMETER_TYPE
GfxDescriptorTypeToDescriptorParamter(gfx_descriptor_type Type)
{
    switch (Type)
    {
        case gfx_descriptor_type::cbv:     return D3D12_ROOT_PARAMETER_TYPE_CBV;
        case gfx_descriptor_type::srv:     return D3D12_ROOT_PARAMETER_TYPE_SRV;
        case gfx_descriptor_type::uav:     return D3D12_ROOT_PARAMETER_TYPE_UAV;
        case gfx_descriptor_type::sampler: // Intentional fallthrough
        default:                           assert(false && "Unsupported descriptor type for a Root Parameter.");
    }

    // this is technically unreachable, but clangd thinks otherwise
    return D3D12_ROOT_PARAMETER_TYPE_CBV;
}

gfx_root_signature::gfx_root_signature(const gfx_device& Device, const gfx_root_signature_info& Info)
{
    // A Root Descriptor can have up to 64 DWORDS, so let's make sure the info fits
    u64 RootSignatureCost = 0;
    RootSignatureCost += Info.DescriptorTables.Length()    * gfx_descriptor_table::cDWordCount;
    RootSignatureCost += Info.Descriptors.Length()         * gfx_root_descriptor::cDWordCount;
    RootSignatureCost += Info.DescriptorConstants.Length() * gfx_root_constant::cDWordCount;

    if (RootSignatureCost > gfx_root_signature::cMaxDWordCount)
    {
#if DEBUG_BUILD
        if (Info.Name.Length())
        {
            LogFatal("Attempting to create a Root Signature with too many descriptors: %d", RootSignatureCost);
        }
        else
#endif
        {
            assert(false && "Too big of a root signature.");
        }
    }

    //
    // Build the root signature description
    // 

    // Ok, so let's just declare a reasonable upper bound here. If we hit it then, uh, let's worry about it then.
    constexpr int MaxDescriptorRanges = 64;
    D3D12_DESCRIPTOR_RANGE1 Ranges[MaxDescriptorRanges] = {};
    u32 TotalDescriptorRangeCount = 0;

    D3D12_ROOT_PARAMETER1 RootParameters[gfx_root_signature::cMaxDWordCount];
    u32 ParameterCount = 0;

    // Collect the descriptor tables
    for (const auto& Table : Info.DescriptorTables)
    {
        D3D12_ROOT_PARAMETER1& Parameter = RootParameters[Table.mRootIndex];
        Parameter.ParameterType          = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        Parameter.ShaderVisibility       = D3D12_SHADER_VISIBILITY_ALL; // TODO(enlynn): make modifiable
        
        assert(TotalDescriptorRangeCount + Table.mDescriptorRanges.Length() < MaxDescriptorRanges);

        u32 BaseRangeIndex = TotalDescriptorRangeCount;
        u64 RangeCount     = Table.mDescriptorRanges.Length();

        bool FoundSampler = false;
        bool FoundCSU     = false; // Found a CBV, UAV, or SRV descriptor range.

        mNumDescriptorsPerTable[Table.mRootIndex] = 0;

        for (const auto& Range : Table.mDescriptorRanges)
        {
            D3D12_DESCRIPTOR_RANGE1& DescriptorRange = Ranges[TotalDescriptorRangeCount];

            switch (Range.mType)
            {
                case gfx_descriptor_type::cbv:     DescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;     FoundCSU     = true; break;
                case gfx_descriptor_type::uav:     DescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;     FoundCSU     = true; break;
                case gfx_descriptor_type::srv:     DescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;     FoundCSU     = true; break;
                case gfx_descriptor_type::sampler: DescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER; FoundSampler = true; break;
                default:                           break;
            }

            switch (Range.mFlags)
            {
                case gfx_descriptor_range_flags::constant:
                { // Data is Constant and Descriptors are Constant. Best for driver optimization.
                    DescriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC;
                } break;

                case gfx_descriptor_range_flags::dynamic:
                { // Data is Volatile and Descriptors are Volatile
                    DescriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE; 
                } break;

                case gfx_descriptor_range_flags::data_constant:
                { // Data is Constant and Descriptors are Volatile
                    DescriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE; 
                } break;

                case gfx_descriptor_range_flags::descriptor_constant:
                { // Data is Volatile and Descriptors are constant
                    DescriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
                } break;

                case gfx_descriptor_range_flags::none: // Intentional Fallthrough                                           
                default:
                { // Default behavior, SRV/CBV are static at execute, UAV are volatile 
                    DescriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
                } break;
            }

            DescriptorRange.NumDescriptors                    = Range.mNumDescriptors;
            DescriptorRange.BaseShaderRegister                = Range.mBaseShaderRegister;
            DescriptorRange.RegisterSpace                     = Range.mRegisterSpace;
            DescriptorRange.OffsetInDescriptorsFromTableStart = Range.mDescriptorOffset;
            DescriptorRange.RangeType                         = GfxDescriptorTypeToD3D12(Range.mType);

            TotalDescriptorRangeCount += 1;

            mNumDescriptorsPerTable[Table.mRootIndex] += Range.mNumDescriptors;
        }

        if (FoundCSU && FoundSampler)
        {
            LogFatal("Found a descriptor table that contained both CBV/SRV/UAV and a Sampler. This is not allowed.");
        }
        else if (FoundCSU)
        { // TODO(enlynn):
            mDescriptorTableBitmask = BitSet(mDescriptorTableBitmask, Table.mRootIndex);
        }
        else if (FoundSampler)
        {// TODO(enlynn):
            //_sampler_table_bitmask |= (1 << i); break;
        }

        // TODO(enlynn): Num Descriptors in a table.

        if (RangeCount > 0)
        {
            Parameter.DescriptorTable.NumDescriptorRanges = (UINT)RangeCount;
            Parameter.DescriptorTable.pDescriptorRanges   = Ranges + BaseRangeIndex;
        }
        else
        {
            Parameter.DescriptorTable.NumDescriptorRanges = 0;
            Parameter.DescriptorTable.pDescriptorRanges   = nullptr;
        }

        ParameterCount += 1;
    }

    // Collect the root (inline) descriptors
    for (const auto& Descriptor : Info.Descriptors)
    {
        D3D12_ROOT_PARAMETER1& Parameter = RootParameters[Descriptor.mRootIndex];
        Parameter.ParameterType             = GfxDescriptorTypeToDescriptorParamter(Descriptor.mType);
        Parameter.ShaderVisibility          = D3D12_SHADER_VISIBILITY_ALL; // TODO(enlynn): make modifiable
        Parameter.Descriptor.RegisterSpace  = Descriptor.mRegisterSpace;
        Parameter.Descriptor.ShaderRegister = Descriptor.mShaderRegister;

        switch (Descriptor.mFlags)
        {
            case gfx_descriptor_range_flags::constant:
            { // Data is Constant and Descriptors are Constant. Best for driver optimization.
                Parameter.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC;
            } break;

            case gfx_descriptor_range_flags::data_constant:
            { // Data is Static.
                Parameter.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC;
            } break;

            case gfx_descriptor_range_flags::descriptor_constant:
            { // Data is Volatile and Descriptors are constant
                Parameter.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE;
            } break;

            case gfx_descriptor_range_flags::dynamic: // Intentional Fallthrough
            case gfx_descriptor_range_flags::none:    // Intentional Fallthrough                                           
            default:
            { // Default behavior, SRV/CBV are static at execute, UAV are volatile 
                Parameter.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
            } break;
        }

        ParameterCount += 1;
    }

    // Collect the root constants
    for (const auto& Constant : Info.DescriptorConstants)
    {
        D3D12_ROOT_PARAMETER1& Parameter = RootParameters[Constant.mRootIndex];
        Parameter.ParameterType            = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        Parameter.ShaderVisibility         = D3D12_SHADER_VISIBILITY_ALL; // TODO(enlynn): make modifiable
        Parameter.Constants.Num32BitValues = Constant.mNum32bitValues;
        Parameter.Constants.RegisterSpace  = Constant.mRegisterSpace;
        Parameter.Constants.ShaderRegister = Constant.mShaderRegister;

        ParameterCount += 1;
    }

    // Collect the static samplers
    // TODO(enlynn): Do it.

    //
    // Create the Root Signature
    //

    // Query the supported Root Signature Version, we want version 1.1
    D3D12_FEATURE_DATA_ROOT_SIGNATURE FeatureData = {};
    FeatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

    if (FAILED(Device.AsHandle()->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &FeatureData, sizeof(FeatureData))))
    {
        FeatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC Desc = {};
    Desc.Version = FeatureData.HighestVersion;
    Desc.Desc_1_1.Flags             = D3D12_ROOT_SIGNATURE_FLAG_NONE; // NOTE(enlynn): Worth denying shader stages?
    Desc.Desc_1_1.NumParameters     = ParameterCount;
    Desc.Desc_1_1.pParameters       = &RootParameters[0];
    Desc.Desc_1_1.NumStaticSamplers = 0;       //anumStaticSamplers;
    Desc.Desc_1_1.pStaticSamplers   = nullptr; //apStaticSamplers;

    // Serialize the root signature.
    ID3DBlob* RootSignatureBlob = nullptr;
    ID3DBlob* ErrorBlob         = nullptr;
    AssertHr(D3D12SerializeVersionedRootSignature(&Desc, &RootSignatureBlob, &ErrorBlob));

    // Create the root signature.
    AssertHr(Device.AsHandle()->CreateRootSignature(0, RootSignatureBlob->GetBufferPointer(), RootSignatureBlob->GetBufferSize(), ComCast(&mHandle)));

#if defined(DEBUG_BUILD)
    //mHandle->SetName(Info.Name.Ptr()); <- Str8 to Str16 conversion
#endif

    mRootParameterCount = ParameterCount;
}

u32 
gfx_root_signature::GetDescriptorTableBitmask(gfx_descriptor_type HeapType) const
{
    u32 Result = 0;
    switch (HeapType)
    {
        case gfx_descriptor_type::srv:     //Intentional Fallthrough
        case gfx_descriptor_type::uav:     //Intentional Fallthrough
        case gfx_descriptor_type::cbv:     Result = mDescriptorTableBitmask; break;
        case gfx_descriptor_type::sampler: Result = mSamplerTableBitmask;    break;
    }
    return Result;
}

u32 
gfx_root_signature::GetNumDescriptors(u32 RootIndex) const
{
    assert(RootIndex < 32);
    return mNumDescriptorsPerTable[RootIndex];
}
