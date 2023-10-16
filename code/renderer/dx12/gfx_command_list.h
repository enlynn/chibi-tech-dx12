#pragma once

#include <types.h>

class gfx_device;
class gfx_resource;

enum class gfx_command_list_type : u8
{
	none,     //error state
	graphics, //standard graphics command list
	compute,  //standard compute command list
	copy,     //standard copy command list
	indirect, //indirect command list
	count,
};

class gfx_command_list
{
public:
	gfx_command_list() = default;
	explicit gfx_command_list(const gfx_device& Device, gfx_command_list_type Type);

	~gfx_command_list() { Release(); }
	void Release();

	inline gfx_command_list_type             GetType()  const { return mType;   }
	inline struct ID3D12GraphicsCommandList* AsHandle() const { return mHandle; }

	void Reset();
	
	void Close();

	// Texture Handling
	void ClearTexture(gfx_resource& TextureResource, f32 ClearColor[4]);

private:
	gfx_command_list_type             mType      = gfx_command_list_type::none;
	struct ID3D12GraphicsCommandList* mHandle    = nullptr;
	struct ID3D12CommandAllocator*    mAllocator = nullptr;
};