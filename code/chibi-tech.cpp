#include "platform/platform.h"
#include "util/str8.h"
#include "util/bit.h"
#include "util/allocator.h"
#include "renderer/simple_renderer.h"

#include "systems/resource_system.h"

#include <string.h>

int main(int ArgumentCount, char* ArgumentList[])
{
	// For now, let's just assume the location of the content path
	assert(ArgumentCount == 2);
	istr8 ContentPath = ArgumentList[1];

	PlatformInit();

	allocator HeapAllocator = allocator::Default();

	//
	// Setup the logging system
	//

	PlatformLogSystemInit();

	log_flags_bitset LogFlags = log_flags_bitset()
		.Set(log_flags::console)
		.Set(log_flags::debug_console);
	PlatformLogSystemSetFlags(LogFlags);

	//
	// Resource System
	// 

	resource_system ResourceSystem = resource_system(ContentPath);

	//
	// Client Window
	//

	platform_window ClientWindow = {};
	ClientWindow.Init("Chibi Tech", 1920, 1080);

	//
	// Renderer
	//

	simple_renderer_info RenderInfo = {
		.HeapAllocator  = &HeapAllocator,
		.ClientWindow   = ClientWindow,
		.ResourceSystem = ResourceSystem,
	};

	SimpleRendererInit(RenderInfo);
	
	//
	// Run the App
	//

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

		SimpleRendererRender();

		// End of the frame
		FrameTimer.Update();
		PerFrameTimer.Update(); 

		// Meet the target frame rate so we don't melt the CPU/GPU
		f64 TargetRefreshRate      = ClientWindow.GetMonitorRefreshRate();
		f64 WorkSecondsElapsed     = PerFrameTimer.GetSecondsElapsed();
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

	SimpleRendererDeinit();
	ClientWindow.Deinit();
	PlatformDeinit();
	PlatformLogSystemDeinit();
	return 0;
}
