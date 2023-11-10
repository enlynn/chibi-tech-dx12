#include "platform.h"
#include <util/str8.h>
#include <util/bit.h>

#include <stdarg.h>
#include <stdio.h>

struct log_message
{
	log_level Level;
	mstr8     Message;
};

struct platform_logger
{
	log_level                                MinLogLevel                             = log_level::trace;
	log_flags_bitset                         Flags                                   = {};
	log_color                                ForegroundColors[(u32)log_level::count] = {
		log_color::blue,    // trace
		log_color::magenta, // debug
		log_color::white,   // info
		log_color::yellow,  // warn
		log_color::red,     // error
		log_color::white,   // fatal
	};
	log_color                                BackgroundColors[(u32)log_level::count] = {
		log_color::black,    // trace
		log_color::black,    // debug
		log_color::black,    // info
		log_color::black,    // warn
		log_color::black,    // error
		log_color::dark_red, // fatal
	};

	// TODO: File I/O and Scrolling. 
	// Instead of storing logs "In Memory" we can just keep an offset into
	// a file. If the editor wants to display logs, we just read from the File.

	// TODO:
	// In Memory Logs if there is an editor to display them, this will be the "log history"
	// or just a hueristic for reading from the Log File
#if 0
	log_message* InMemoryLogs;
	u32          InMemoryLogMax;
	u32          InMemoryLogFirst;
	u32          InMemoryLogLast;
#endif

	// TODO: Threading
	// Proper synchronization for logs
};

var_global platform_logger gLogger = {};

void 
PlatformLogSystemInit([[maybe_unused]] int InMemoryLogCount)
{
	// NOTE(enlynn): since I am currently not setting up File I/O, In Memory Logs, or Thread Sync,
	// there is nothing to do right now!
}

void 
PlatformLogSystemDeinit()
{
	// NOTE(enlynn): nothing for now.
}

void 
PlatformLogSystemMinLogLevel(log_level MinLogLevel)
{
	gLogger.MinLogLevel = MinLogLevel;
}

void 
PlatformLogSystemSetLogColor(log_level Level, log_color Foreground, log_color Background)
{
	gLogger.ForegroundColors[(u32)Level] = Foreground;
	gLogger.BackgroundColors[(u32)Level] = Background;
}

void 
PlatformLogSystemSetFlags(const log_flags_bitset& Flags)
{
	gLogger.Flags = Flags;
}


void 
PlatformLogSystemLog(log_level LogLevel, const char* File, int Line, const char* Format, ...)
{
	assert(LogLevel < log_level::count);
	if (LogLevel < gLogger.MinLogLevel) return; 

	// [TRACE] [Thread] File:Line : Message
	// --- TODO: write out the Stack Trace
	// [Debug] [Thread] File:Line : Message
	// [Info] Message
	// [Warn] Message
	// [Error] [Thread] File:Line : Message
	// [Fatal] [Thread] File:Line : Message

	const char* LogLevelNames[u32(log_level::count)] = {
		"Trace",
		"Debug",
		"Info",
		"Warn",
		"Error",
		"Fatal"
	};

	const char* LevelName = LogLevelNames[u32(LogLevel)];

    constexpr int HardcodedMessageSize = 2048;
	char PartialMessage[HardcodedMessageSize];

	va_list MessageArgs;
	va_start(MessageArgs, Format);

	int MessageLength = vsnprintf(PartialMessage, HardcodedMessageSize, Format, MessageArgs);
	assert(MessageLength < HardcodedMessageSize); // TODO: Are exceeding hard coded limits, prolly should do something better

	PartialMessage[MessageLength]     = '\n';
	PartialMessage[MessageLength + 1] = 0;

	va_end(MessageArgs);

	mstr8 FullMessage = "[";
	FullMessage = FullMessage + LevelName + "] ";

	if (LogLevel != log_level::info && LogLevel != log_level::warn)
	{
		FullMessage += mstr8::Format("%s:%d ", File, Line);
	}

	FullMessage += PartialMessage;

	if (gLogger.Flags.IsSet(log_flags::file))
	{ //TODO:
	}

	if (gLogger.Flags.IsSet(log_flags::editor))
	{ //TODO:
	}

	if (gLogger.Flags.IsSet(log_flags::console))
	{
		PlatformLogToConsole(LogLevel > log_level::warn,
			gLogger.ForegroundColors[u32(LogLevel)],
			gLogger.BackgroundColors[u32(LogLevel)],
			FullMessage);
	}

#if _DEBUG
	if (gLogger.Flags.IsSet(log_flags::debug_console))
	{
		PlatformLogToDebugConsole(FullMessage);
	}
#endif

	if (LogLevel == log_level::fatal)
	{
		PlatformExitProcess();
	}
}