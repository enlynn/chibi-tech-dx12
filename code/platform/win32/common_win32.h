#ifndef _PLATFORM_WIN32_COMMON_
#define _PLATFORM_WIN32_COMMON_

#include <types.h>

#include "../platform.h"

#include <Windows.h>
#include <windowsx.h>
#include <timeapi.h>

// Caller is responsible for freeing the utf16 string
wchar_t* Win32Utf8ToUtf16(class allocator& Allocator, const char* Utf8String, u64 Utf8StringSize);

#endif //_PLATFORM_WIN32_COMMON_