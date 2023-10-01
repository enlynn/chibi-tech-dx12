#include "common_win32.h"

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