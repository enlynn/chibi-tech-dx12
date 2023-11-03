#include "common_win32.h"

#include <util/allocator.h>
#include <util/str8.h>

bool 
PlatformLoadFileIntoBuffer(allocator& Allocator, istr8 AbsolutePath, u8** Buffer, u64* BufferSize)
{
	wchar_t* FilePathWide = Win32Utf8ToUtf16(Allocator, AbsolutePath.Ptr(), AbsolutePath.Length());

	HANDLE FileHandle = CreateFileW(FilePathWide, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (FileHandle == INVALID_HANDLE_VALUE)
	{
		DWORD Error = GetLastError();
		LogFatal("Unable to open file: %s, with error: %d", AbsolutePath.Ptr(), Error);
		CloseHandle(FileHandle);
		return false;
	}

	// Get the file info so we can know the size of the file
	BY_HANDLE_FILE_INFORMATION FileInfo = {};
	BOOL FileInfoResult = GetFileInformationByHandle(FileHandle, &FileInfo);
	assert(FileInfoResult != 0);

	// Not going to properly composite the high and low order size bits, and
	// not going to read in files greater than U32_MAX.

	// If this ever happens, need to do something special. Can't just dump a file this large into memory.
	assert(FileInfo.nFileSizeHigh == 0); 

	*BufferSize = FileInfo.nFileSizeLow;
	*Buffer = (u8*)Allocator.AllocChunk(*BufferSize);

	DWORD BytesRead;
	BOOL FileReadResult = ReadFile(FileHandle, *Buffer, DWORD(*BufferSize), &BytesRead, nullptr);
	assert(FileReadResult != FALSE);

	Allocator.Free(FilePathWide);
	CloseHandle(FileHandle);
	return true;
}