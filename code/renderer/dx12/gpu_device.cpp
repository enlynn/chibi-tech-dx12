#include "gpu_device.h"
#include "d3d12_common.h"
#include "gpu_utils.h"
#include <platform/platform.h>

#include <util/str8.h>
#include <util/str16.h>

var_global const D3D_FEATURE_LEVEL gMinFeatureLevel = D3D_FEATURE_LEVEL_12_0;

void DeviceValidationCallback(
    D3D12_MESSAGE_CATEGORY Category,
    D3D12_MESSAGE_SEVERITY Severity,
    D3D12_MESSAGE_ID ID,
    LPCSTR pDescription,
    void* pContext)
{
    LogWarn("This is a validation call.");
}

void 
gpu_device::Init()
{
	EnableDebugDevice();
	SelectAdapter();

	// Create the device

	AssertHr(D3D12CreateDevice(mAdapter, gMinFeatureLevel, ComCast(&mDevice)));

	ID3D12InfoQueue* InfoQueue = nullptr;
	if (SUCCEEDED(mDevice->QueryInterface(ComCast(&InfoQueue))))
	{
		InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);

		// Suppress messages based on severity level
		D3D12_MESSAGE_SEVERITY Severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
		
		D3D12_MESSAGE_ID HideMessages[] = {
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,  // I'm really not sure how to avoid this message.
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
		};

		D3D12_INFO_QUEUE_FILTER MessageFilter = {};
		MessageFilter.DenyList.NumSeverities = ArrayCount(Severities);
		MessageFilter.DenyList.pSeverityList = Severities;
		MessageFilter.DenyList.NumIDs = ArrayCount(HideMessages);
		MessageFilter.DenyList.pIDList = HideMessages;
		InfoQueue->PushStorageFilter(&MessageFilter);

        ID3D12InfoQueue1* InfoQueue1 = nullptr;
        if (SUCCEEDED(mDevice->QueryInterface(ComCast(&InfoQueue1))))
        {
            DWORD CallbackIdentifier;
            AssertHr(InfoQueue1->RegisterMessageCallback(
                DeviceValidationCallback,
                D3D12_MESSAGE_CALLBACK_FLAG_NONE,
                nullptr,
                &CallbackIdentifier
            ));
            ComSafeRelease(InfoQueue1);
        }

		ComSafeRelease(InfoQueue);
	}

	const D3D_FEATURE_LEVEL SupportedFeatureLevels[] = {
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0, // TODO(enlynn): Check to determine what the min feature level should be
	};

	const D3D12_FEATURE_DATA_FEATURE_LEVELS FeatureLevels = {
		.NumFeatureLevels = ArrayCount(SupportedFeatureLevels),
		.pFeatureLevelsRequested = SupportedFeatureLevels,
		.MaxSupportedFeatureLevel = D3D_FEATURE_LEVEL_12_2,
	};

	HRESULT Hr = mDevice->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, (void**) & FeatureLevels, sizeof(FeatureLevels));
	if (SUCCEEDED(Hr))
	{
		mSupportedFeatureLevel = FeatureLevels.MaxSupportedFeatureLevel;
	}
	else
	{
		mSupportedFeatureLevel = gMinFeatureLevel;
	}
}

void 
gpu_device::Deinit()
{
	ComSafeRelease(mDevice);
	ComSafeRelease(mAdapter);

	ReportLiveObjects();
}

void 
gpu_device::EnableDebugDevice()
{
#ifdef _DEBUG
	ID3D12Debug* DebugController0;
	if (FAILED(D3D12GetDebugInterface(ComCast(&DebugController0))))
	{
		return;
	}

	ID3D12Debug1* DebugController1;
	if (FAILED(DebugController0->QueryInterface(ComCast(&DebugController1))))
	{
		return;
	}

	DebugController1->EnableDebugLayer();
	DebugController1->SetEnableGPUBasedValidation(true);

	LogDebug("Successfully initialized D3D12 Debug Layer.");
#endif
}

void
gpu_device::ReportLiveObjects()
{
#if _DEBUG
	IDXGIDebug1* Debug = nullptr;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, ComCast(&Debug))))
	{
		Debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		ComSafeRelease(Debug);
	}
#endif
}

void 
gpu_device::SelectAdapter()
{
	IDXGIFactory6* Factory6 = nullptr;
	IDXGIAdapter1* Adapter  = nullptr;

	u32 FactoryFlags = 0;
#if _DEBUG
	FactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	AssertHr(CreateDXGIFactory2(FactoryFlags, ComCast(&Factory6)));

	// Select the adapter
	u32 AdapterId = 0;
	HRESULT HrResult = Factory6->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, ComCast(&Adapter));
	while (HrResult != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC1 Desc1 = {};
		Adapter->GetDesc1(&Desc1);

		// Convert the description to UTF8
		istr16 IDevice    = istr16((c16*)&Desc1.Description[0]);
		mstr8  DeviceInfo = mstr8(&IDevice);

		LogInfo("%s", DeviceInfo.Ptr());

		ComSafeRelease(Adapter);

		AdapterId += 1;
		Factory6->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, ComCast(&Adapter));
	
		if (Desc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{ // Don't select the software device. This should be the fallback.
			continue;
		}

		// Check to see if the adapter can create a D3D12 device without actually 
		// creating it. The adapter with the largest dedicated video memory
		// is favored.
		if (SUCCEEDED(D3D12CreateDevice(Adapter, gMinFeatureLevel, ExplicitComCast(ID3D12Device, nullptr))))
		{
			break;
		}
	}

#ifdef _DEBUG
	if (!Adapter)
	{ // Failed to find a Graphics Device, fallback to WinAPIs WARP Renderer (Software Rasterizer)
		AssertHr(Factory6->EnumWarpAdapter(ComCast(&Adapter)));
	}
#endif

	mAdapter = Adapter;

	ComSafeRelease(Factory6);
}

ID3D12DescriptorHeap* 
gpu_device::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type, u32 Count, bool IsShaderVisible)
{
	ID3D12DescriptorHeap* Result = nullptr;

	D3D12_DESCRIPTOR_HEAP_DESC Desc = {};
	Desc.Type                       = Type;
	Desc.NumDescriptors             = Count;
	Desc.Flags                      = (IsShaderVisible) ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	AssertHr(mDevice->CreateDescriptorHeap(&Desc, ComCast(&Result)));

	return Result;
}

gpu_resource
gpu_device::CreateCommittedResource(commited_resource_info& Info)
{
    ID3D12Resource* TempResource = nullptr;

    D3D12_HEAP_PROPERTIES HeapProperties = GetHeapProperties(Info.HeapType);
    D3D12_RESOURCE_DESC   BufferDesc     = GetBufferResourceDesc(Info.Size, Info.ResourceFlags, Info.Alignment);

    AssertHr(mDevice->CreateCommittedResource(
        &HeapProperties,
        Info.HeapFlags,
        &BufferDesc,
        Info.InitialState,
        Info.ClearValue,
        ComCast(&TempResource))
    );

    return gpu_resource(*this, TempResource);
}
