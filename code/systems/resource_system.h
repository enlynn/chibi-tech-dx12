#pragma once

#include <types.h>
#include <util/str8.h>
#include <util/allocator.h>

enum class resource_type : u8
{
	builtin_shader,
	custom,
	count,
	unknown = count,
};

// Parent Struct
struct resource
{
	resource_type mType         = resource_type::unknown; // Loader Id
	
	void*         mBaseData     = nullptr;                // Unparsed file data. Owned by the resource_loader.
	u64           mBaseDataSize = 0;                      // The resource type is responsible for parsing the data

	resource() {}
	explicit resource(resource_type Type, void* Data, u64 DataSize) : mType(Type), mBaseData(Data), mBaseDataSize(DataSize) {}
	// implementations can choose to not use the inheritance approach. if so, don't override this function.
	virtual bool Parse(struct resource_loader* Self, const char* Name, resource* OutResource) { return true; }
};

struct resource_loader
{
	resource_type mType         = resource_type::unknown;
	mstr8         mCustomName   = {}; // Name for Custom Resource Types, TODO(enlynn): support
	mstr8         mRelativePath = {};

	bool (*Load)(resource_loader* Self, const istr8 AbsolutePath, const istr8 ResourceName, resource* OutResource);
	void (*Unload)(resource_loader* self, resource* resource);
};

class resource_system
{
public:
	resource_system(istr8 BasePath);

	void RegisterLoader(resource_loader& Loader);

	// Loads a builtin resource type
	bool Load(resource_type Type, istr8 ResourceName, resource* OutResource);
	void Unload(resource_type Type, resource* InResource);

	// Loads a custom resource type - TODO(enlynn)
	//bool LoadCustom  (const istr8 ResourceName, resource* OutResource);
	//void UnloadCustom(const istr8 ResourceName, resource* InResource);

private:
	struct resource_loader_entry 
	{
		mstr8           mAbsolutePath = {};
		resource_loader mLoader       = {};
	};

	mstr8                 mBasePath                               = {};
	resource_loader_entry mLoaders[u32(resource_type::count) - 1] = {}; // Don't store custom loaders.
};

#if 0 // TODO:
class resource_watcher
{
};
#endif

#if 0 // Usage code
ResourceSystem->RegisterLoader(shader_resource::GetResourceLoader());

shader_resource Resource = {};
ResourceSystem->Load(resource_type::builtin_shader, "SimpleVertex.Vtx", &shader_resource);
// Do stuff...
ResourceSystem->Unload(&Resource);
#endif