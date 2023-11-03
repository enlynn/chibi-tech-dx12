#include "resource_system.h"

fn_internal void
NormalizePath(mstr8& Path)
{
	ForRange(u64, i, Path.Length())
	{
		if (Path[i] == '\\')
			Path[i] = '/';
	}
	Path.ShrinkToFit();
}

resource_system::resource_system(istr8 BasePath)
{
	mBasePath = mstr8(BasePath);
	NormalizePath(mBasePath);
}

void 
resource_system::RegisterLoader(resource_loader& Loader)
{
	assert(Loader.mType != resource_type::unknown);

	if (Loader.mType < resource_type::custom)
	{
		u32 LoaderIndex = u32(Loader.mType);
		mLoaders[LoaderIndex].mLoader = Loader;

		u64 Length = mLoaders[LoaderIndex].mLoader.mRelativePath.Length();

		NormalizePath(mLoaders[LoaderIndex].mLoader.mRelativePath);

		Length = mLoaders[LoaderIndex].mLoader.mRelativePath.Length();

		// Convert the relative path to an absolute path.
		mLoaders[LoaderIndex].mAbsolutePath = mBasePath + "/" + mLoaders[LoaderIndex].mLoader.mRelativePath;
	}
	else
	{
		// TODO(enlynn): Handle custom loaders
	}
}

bool 
resource_system::Load(resource_type Type, istr8 ResourceName, resource* OutResource)
{
	assert(Type != resource_type::unknown);

	if (Type < resource_type::custom)
	{
		resource_loader& Loader = mLoaders[u32(Type)].mLoader;
		return Loader.Load(&Loader, mLoaders[u32(Type)].mAbsolutePath, ResourceName, OutResource);
	}
	else
	{
		// TODO(enlynn): Handle custom loaders
	}

	return false;
}

void 
resource_system::Unload(resource_type Type, resource* InResource)
{
	assert(Type != resource_type::unknown);

	if (Type < resource_type::custom)
	{
		resource_loader& Loader = mLoaders[u32(Type)].mLoader;
		Loader.Unload(&Loader, InResource);
	}
	else
	{
		// TODO(enlynn): Handle custom loaders
	}
}