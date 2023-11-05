#pragma once

#include <systems/resource_system.h>

#include "d3d12_common.h"

using gpu_shader_blob = ID3DBlob*;
struct gpu_shader_modules
{
	gpu_shader_blob Vertex  = nullptr;
	gpu_shader_blob Pixel   = nullptr;
	gpu_shader_blob Compute = nullptr;
	// TODO: Handle mesh shading and RT pipeline
};

enum class shader_stage
{
	unknown,
	vertex,
	pixel,
	compute,
	count,
};

struct shader_resource : public resource
{
	shader_resource(shader_stage ShaderStage) : mStage(ShaderStage), resource(sType, nullptr, 0)  {}
	
	// For a shader resource, this doesn't do anything. Nothing to parse.
	// Don't need to override Parse because all it would do is make a copy of Data and DataSize

	D3D12_SHADER_BYTECODE GetShaderBytecode();

	static resource_loader GetResourceLoader();

	static bool Load(resource_loader* Self, const istr8 AbsolutePath, const istr8 ResourceName, resource* OutResource);
	static void Unload(resource_loader* self, resource* resource);

	static constexpr resource_type sType = resource_type::builtin_shader;
	static constexpr const char* sPath = "shader/bin";

	shader_stage    mStage      = shader_stage::unknown;
	gpu_shader_blob mShaderBlob = nullptr;
};