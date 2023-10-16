#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include <types.h>
#include <util/bit.h>

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

	platform_window_handle       GetHandle()   const; //windows=HWND
	platform_window_instance     GetInstance() const; //windows=HINSTANCE

	platform_window_rect         GetWindowRect() const;
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

enum class log_flags : u8
{
	none          = 0,
	file          = 1,
	editor        = 2,
	console       = 3,
	debug_console = 4,
	count,
};

using log_flags_bitset = bitset<log_flags, (u32)log_flags::count>;

enum class log_level : u8
{
	trace,
	debug,
	info,
	warn,
	error,
	fatal,
	count,
};

enum class log_color : u8
{
	black,
	dark_blue,
	dark_green,
	dark_cyan,
	dark_red,
	dark_magenta,
	dark_yellow,
	grey,
	dark_grey,
	blue,
	green,
	cyan,
	red,
	magenta,
	yellow,
	white,
	max,
};

#define LogTrace(Fmt, ...) PlatformLogSystemLog(log_level::trace, __FILE__, __LINE__, Fmt, ##__VA_ARGS__)
#define LogDebug(Fmt, ...) PlatformLogSystemLog(log_level::debug, __FILE__, __LINE__, Fmt, ##__VA_ARGS__)
#define LogInfo(Fmt,  ...) PlatformLogSystemLog(log_level::info,  __FILE__, __LINE__, Fmt, ##__VA_ARGS__)
#define LogWarn(Fmt,  ...) PlatformLogSystemLog(log_level::warn,  __FILE__, __LINE__, Fmt, ##__VA_ARGS__)
#define LogError(Fmt, ...) PlatformLogSystemLog(log_level::error, __FILE__, __LINE__, Fmt, ##__VA_ARGS__)
#define LogFatal(Fmt, ...) PlatformLogSystemLog(log_level::fatal, __FILE__, __LINE__, Fmt, ##__VA_ARGS__)

// Platform Independent Log Implementation.
void PlatformLogSystemInit(int InMemoryLogCount = 50);
void PlatformLogSystemDeinit();
void PlatformLogSystemMinLogLevel(log_level MinLogLevel = log_level::trace);
void PlatformLogSystemSetFlags(const log_flags_bitset& Flags);
void PlatformLogSystemSetLogColor(log_level Level, log_color Foreground, log_color Background);
void PlatformLogSystemLog(log_level LogLevel, const char* File, int Line, const char* Format, ...);

// Platform Dependent Log Implementation
void PlatformLogToConsole(bool IsError, log_color Foreground, log_color Background, const struct istr8& Message);
void PlatformLogToDebugConsole(const struct istr8& Message);


#endif //_PLATFORM_H_