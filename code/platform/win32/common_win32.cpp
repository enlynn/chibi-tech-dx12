#include "common_win32.h"

#include <util/allocator.h>

void PlatformInit()
{
	UINT DesiredSchedulerMS = 1;
	bool _SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);
}

void PlatformDeinit()
{ // NOTE: nothing for now
}

void PlatformSleepMainThread(u32 TimeMS)
{
	Sleep(TimeMS);
}

void PlatformExitProcess()
{
	// TODO: proper exit code?
	ExitProcess(0);
}

wchar_t* 
Win32Utf8ToUtf16(allocator& Allocator, const char* Utf8String, u64 Utf8StringSize)
{
	// First pass in a null bufferr to get the required size.
	int NumberChars = MultiByteToWideChar(CP_UTF8, 0, Utf8String, int(Utf8StringSize), nullptr, 0);
	assert(NumberChars != 0); // If we get 0, then something went wrong.

	wchar_t* Utf16String = (wchar_t*)Allocator.AllocChunk((u64(NumberChars) + 1) * sizeof(wchar_t));
	Utf16String[NumberChars] = L'\0';

	// Convert the string for real this time
	NumberChars = MultiByteToWideChar(CP_UTF8, 0, Utf8String, int(Utf8StringSize), Utf16String, (size_t)NumberChars);
	assert(NumberChars != 0); // If we get 0, then something went wrong.
	
	return (wchar_t*)Utf16String;
}