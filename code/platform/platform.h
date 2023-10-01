#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include <types.h>

//
// Common Platform Functions
//

void PlatformInit();
void PlatformDeinit();

void PlatformSleepMainThread(u32 TimeMS);

//
// Platform Window
//

struct platform_window_rect
{
	u32 Width;
	u32 Height;
	u32 X;
	u32 Y;
};

struct platform_pump_message_result
{
	b8 Error      : 1 = 0;
	b8 Resize     : 1 = 0;
	b8 Fullscreen : 1 = 0;
	b8 Close      : 1 = 0;
};

struct platform_window_state
{
	b8 IsRunning     : 1 = 0;
	b8 IsShown       : 1 = 0;
	b8 IsFullscreen  : 1 = 0;
	b8 NeedsResized  : 1 = 0;
};

using platform_window_handle   = void*;
using platform_window_instance = void*;

class platform_window
{
public:
	void                         SetIsRunning(const bool IsRunning)       { mWindowState.IsRunning = IsRunning; }
	bool                         IsRunning()                        const { return mWindowState.IsRunning == 1; }
	
	bool                         IsResized()                        const { return mWindowState.NeedsResized;   }
	void                         SetNeedsResized()                        { mWindowState.NeedsResized = 1;      }
	void                         SetResizedHandled()                      { mWindowState.NeedsResized = 0;      }

	bool                         IsFullscreen()                     const { return mWindowState.IsFullscreen;   }

	f64                          GetMonitorRefreshRate()            const { return mMonitorRefreshRate;         }

	void                         Init(const char* Name, u32 Width, u32 Height);
	void                         Deinit();

	platform_window_handle       GetHandle();   //windows=HWND
	platform_window_instance     GetInstance(); //windows=HINSTANCE

	platform_window_rect         GetWindowRect();
	// 
	// TODO(enlynn):
	// - If I choose to use IMGUI, then it might be a good idea for a user to supply a custom "PumpMessages"
	//   to the window abstractions.
	// - InputSystem
	//
	platform_pump_message_result PumpMessages();

private:
	f64                   mMonitorRefreshRate = 0.0;
	platform_window_state mWindowState        = {};
	void*                 mInternalState      = nullptr;
};

//
// Platform Timer
//

class platform_timer
{
public:
	platform_timer();

	void Start();
	void Update();

	f64 GetSecondsElapsed()     const;
	f64 GetMiliSecondsElapsed() const { return 1000.0 * GetSecondsElapsed(); }

private:
	f64 mFrequency   = 0.0;
	f64 mStartTime   = 0.0;
	f64 mElapsedTime = 0.0;
};


//
// Platform Logger
//

#define LogTrace
#define LogDebug
#define LogInfo
#define LogWarn
#define LogError
#define LogFatal



#endif //_PLATFORM_H_