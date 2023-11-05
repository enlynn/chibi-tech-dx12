#include "gpu_shader_utils.h"

#include <platform/platform.h>

D3D12_SHADER_BYTECODE 
shader_resource::GetShaderBytecode()
{
	D3D12_SHADER_BYTECODE Bytecode  = {};
	Bytecode.BytecodeLength  = mBaseDataSize;
	Bytecode.pShaderBytecode = mBaseData;
	return Bytecode;
}

resource_loader 
shader_resource::GetResourceLoader()
{
	resource_loader Loader = {};
	Loader.mCustomName     = "builtin-shaders";
	Loader.mRelativePath   = "shaders/out";
	Loader.Load            = Load;
	Loader.Unload          = Unload;
	Loader.mType           = resource_type::builtin_shader;
	return Loader;
};

bool 
shader_resource::Load(resource_loader* Self, const istr8 AbsolutePath, const istr8 ResourceName, resource* OutResource)
{
	shader_resource* ShaderResource = (shader_resource*)OutResource;

	mstr8 FilePath = mstr8(AbsolutePath) + "/" + ResourceName;
	if (ShaderResource->mStage == shader_stage::vertex)
	{
		FilePath += ".Vtx.cso";
	}
	else if (ShaderResource->mStage == shader_stage::pixel)
	{
		FilePath += ".Pxl.cso";
	}
	else if (ShaderResource->mStage == shader_stage::compute)
	{
		FilePath += ".Cpt.cso";
	}

	allocator FileAllocator = allocator::Default(); // TODO(enlynn): assign a proper allocator.
	PlatformLoadFileIntoBuffer(FileAllocator, FilePath, (u8**)&OutResource->mBaseData, &OutResource->mBaseDataSize);
	
	return ShaderResource->Parse(Self, ResourceName, OutResource);
}

void 
shader_resource::Unload(resource_loader* Self, resource* Resource)
{
	allocator FileAllocator = allocator::Default(); // TODO(enlynn): assign a proper allocator.
	FileAllocator.Free(Resource->mBaseData);
}