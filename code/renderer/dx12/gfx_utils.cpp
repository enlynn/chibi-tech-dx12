#include "gfx_utils.h"

D3D12_HEAP_PROPERTIES GetHeapProperties(D3D12_HEAP_TYPE Type)
{
    D3D12_HEAP_PROPERTIES Result = {};
    Result.Type                  = Type;
    Result.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    Result.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;
    Result.CreationNodeMask      = 1;
    Result.VisibleNodeMask       = 1;
    return Result;
}