﻿#include "chibi-tech.h"
#include "platform/platform.h"
#include "util/str8.h"

fn_internal void
TestStrings()
{
	// Basic default constructor.
	mstr8 StringA = {};
	assert(StringA.Length()   == 0);
	assert(StringA.Capacity() == 23);
	assert(!StringA.IsHeap());

	mstr8 StringB = {};
	StringB.SetLength(23);
	assert(StringB.Length()   == 23);
	assert(StringB.Capacity() == 23);
	assert(!StringB.IsHeap());

	mstr8 StringC = {};
	StringC.SetLength(24);
	assert(StringC.Length()   == 24);
	assert(StringC.IsHeap());
	
	StringC.SetLength(0);
	StringC.ShrinkToFit();
	assert(StringC.Length()   == 0);
	assert(StringC.Capacity() == 23);
	assert(!StringC.IsHeap());

	// Stack strings
	mstr8 StringStackA = mstr8("123");
	assert(StringStackA.Length()   == 3);
	assert(StringStackA.Capacity() == 23);
	assert(!StringStackA.IsHeap());

	mstr8 StringStackB = mstr8("abcdefghijklmoooooooooo");
	assert(StringStackB.Length()   == 23);
	assert(StringStackB.Capacity() == 23);
	assert(!StringStackB.IsHeap());

	mstr8 StringHeapA = mstr8("abcdefghijklmooooooooooo");
	assert(StringHeapA.Length() == 24);
	assert(StringHeapA.Capacity() == 32);
	assert(StringHeapA.IsHeap());

	mstr8 StringAddA = {};
	StringAddA += 'a';

	assert(!StringAddA.IsHeap());
	assert(strcmp(StringAddA.Ptr(), "a") == 0);

	StringAddA = StringAddA + "abc";
	assert(!StringAddA.IsHeap());

	const char* TempStr = StringAddA.Ptr();
	assert(strcmp(TempStr, "aabc") == 0);

	StringAddA.Prepend("hi");
	assert(strcmp(StringAddA.Ptr(), "hiaabc") == 0);

	StringAddA.Append("hi");
	assert(strcmp(StringAddA.Ptr(), "hiaabchi") == 0);

	StringAddA.SetLength(0);
	StringAddA.ShrinkToFit();

	StringAddA += "helloworld";
	StringAddA.Insert(5, " ");
	assert(strcmp(StringAddA.Ptr(), "hello world") == 0);

	StringAddA.Remove(5, 1);
	assert(strcmp(StringAddA.Ptr(), "helloworld") == 0);

	mstr8 StringAddB = "llo";
	StringAddB = "he" + StringAddB + " world";
	assert(strcmp(StringAddB.Ptr(), "hello world") == 0);

	mstr8 StringAddC = "hello";
	mstr8 StringAddD = "world";
	mstr8 StringAddE = StringAddC + " " + StringAddD;
	assert(strcmp(StringAddE.Ptr(), "hello world") == 0);
}

int main()
{
	PlatformInit();

	TestStrings();

	platform_window ClientWindow = {};
	ClientWindow.Init("Chibi Tech", 1920, 1080);

	platform_timer FrameTimer    = {};
	platform_timer PerFrameTimer = {};

	FrameTimer.Start();
	while (ClientWindow.IsRunning())
	{
		PerFrameTimer.Start();

		platform_pump_message_result PumpResult = ClientWindow.PumpMessages();
		if (PumpResult.Close || PumpResult.Error)
		{
			continue;
		}

		if (PumpResult.Fullscreen)
		{
		}

		if (PumpResult.Resize)
		{
		}

		// End of the frame
		FrameTimer.Update();
		PerFrameTimer.Update(); 

		// Meet the target frame rate so we don't melt the CPU/GPU
		f64 TargetRefreshRate = ClientWindow.GetMonitorRefreshRate();
		f64 WorkSecondsElapsed = PerFrameTimer.GetSecondsElapsed();
		f64 SecondsElapsedForFrame = WorkSecondsElapsed;
		if (SecondsElapsedForFrame < TargetRefreshRate)
		{
			s64 SleepMS = (s64)(1000.0 * (TargetRefreshRate - SecondsElapsedForFrame));
			if (SleepMS > 0)
			{
				PlatformSleepMainThread((u32)SleepMS);
			}
		}
	}

	ClientWindow.Deinit();
	PlatformDeinit();
	return 0;
}
