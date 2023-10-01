#include "common_win32.h"
#include <util/str8.h>

#include <cstdio>

struct win32_standard_stream
{
    HANDLE Handle;                         // Stream handle (STD_OUTPUT_HANDLE or STD_ERROR_HANDLE).
    bool   IsRedirected;                   // True if redirected to file.
};

#if 0 // From wincon.h, here for reference
#define FOREGROUND_BLUE            0x0001
#define FOREGROUND_GREEN           0x0002
#define FOREGROUND_RED             0x0004
#define FOREGROUND_INTENSITY       0x0008
#define BACKGROUND_BLUE            0x0010
#define BACKGROUND_GREEN           0x0020
#define BACKGROUND_RED             0x0040
#define BACKGROUND_INTENSITY       0x0080
#define COMMON_LVB_LEADING_BYTE    0x0100
#define COMMON_LVB_TRAILING_BYTE   0x0200
#define COMMON_LVB_GRID_HORIZONTAL 0x0400
#define COMMON_LVB_GRID_LVERTICAL  0x0800
#define COMMON_LVB_GRID_RVERTICAL  0x1000
#define COMMON_LVB_REVERSE_VIDEO   0x4000
#define COMMON_LVB_UNDERSCORE      0x8000
#endif

// Based on .NET ConsoleColor enumeration
// Foreground and Background colors are essentially the same bitfield, except the Background colors
// are left shifted 4 bits. Win32ConsoleColor can be composed by doing:
//     console_color = foreground | (background << 4)
enum class Win32ConsoleColor : int
{
    black        = 0,                                                   // color = black        | value = 0
    dark_blue    = FOREGROUND_BLUE,                                     // color = dark blue    | value = 1
    dark_green   = FOREGROUND_GREEN,                                    // color = dark green   | value = 2
    dark_cyan    = FOREGROUND_BLUE | FOREGROUND_GREEN,                  // color = dark cyan    | value = 3
    dark_red     = FOREGROUND_RED,                                      // color = dark red     | value = 3
    dark_magenta = FOREGROUND_BLUE | FOREGROUND_RED,                    // color = dark magenta | value = 5
    dark_yellow  = FOREGROUND_RED | FOREGROUND_GREEN,                   // color = dark yellow  | value = 6
    grey         = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED, // color = grey         | value = 7
    dark_grey    = FOREGROUND_INTENSITY,                                // color = dark grey    | value = 8
    blue         = FOREGROUND_INTENSITY | dark_blue,                    // color = blue         | value = 9
    green        = FOREGROUND_INTENSITY | dark_green,                   // color = green        | value = 10
    cyan         = FOREGROUND_INTENSITY | dark_cyan,                    // color = cyan         | value = 11
    red          = FOREGROUND_INTENSITY | dark_red,                     // color = red          | value = 12
    magenta      = FOREGROUND_INTENSITY | dark_magenta,                 // color = magenta      | value = 13
    yellow       = FOREGROUND_INTENSITY | dark_yellow,                  // color = yellow       | value = 14
    white        = FOREGROUND_INTENSITY | grey,                         // color = white        | value = 15
};

fn_inline WORD 
Win32ComposeConsoleColor(Win32ConsoleColor Foreground, Win32ConsoleColor Background)
{
    return int(Foreground) | (int(Background) << 4);
}

fn_inline WORD 
Win32GetConsoleColor(HANDLE console_handle)
{
    WORD Result = Win32ComposeConsoleColor(Win32ConsoleColor::white, Win32ConsoleColor::black);

    CONSOLE_SCREEN_BUFFER_INFO console_info{};
    BOOL info_Result = GetConsoleScreenBufferInfo(console_handle, &console_info);
    if (info_Result > 0)
    {
        Result = console_info.wAttributes;
    }

    return Result;
}

fn_internal Win32ConsoleColor 
LogColorToWin32Color(log_color in_color)
{
    Win32ConsoleColor OutColor = Win32ConsoleColor::black;
    switch (in_color)
    {
    case log_color::black:        OutColor = Win32ConsoleColor::black;        break;
    case log_color::dark_blue:    OutColor = Win32ConsoleColor::dark_blue;    break;
    case log_color::dark_green:   OutColor = Win32ConsoleColor::dark_green;   break;
    case log_color::dark_cyan:    OutColor = Win32ConsoleColor::dark_cyan;    break;
    case log_color::dark_red:     OutColor = Win32ConsoleColor::dark_red;     break;
    case log_color::dark_magenta: OutColor = Win32ConsoleColor::dark_magenta; break;
    case log_color::dark_yellow:  OutColor = Win32ConsoleColor::dark_yellow;  break;
    case log_color::grey:         OutColor = Win32ConsoleColor::grey;         break;
    case log_color::dark_grey:    OutColor = Win32ConsoleColor::dark_grey;    break;
    case log_color::blue:         OutColor = Win32ConsoleColor::blue;         break;
    case log_color::green:        OutColor = Win32ConsoleColor::green;        break;
    case log_color::cyan:         OutColor = Win32ConsoleColor::cyan;         break;
    case log_color::red:          OutColor = Win32ConsoleColor::red;          break;
    case log_color::magenta:      OutColor = Win32ConsoleColor::magenta;      break;
    case log_color::yellow:       OutColor = Win32ConsoleColor::yellow;       break;
    case log_color::white:        OutColor = Win32ConsoleColor::white;        break;
    }
    return OutColor;
}

fn_internal bool 
Win32RedirectConsoleIo()
{
    bool Result = true;
    FILE* fp;

    // Redirect STDIN if the console has an input handle
    if (GetStdHandle(STD_INPUT_HANDLE) != INVALID_HANDLE_VALUE)
    {
        if (freopen_s(&fp, "CONIN$", "r", stdin) != 0)
            Result = false;
    }
    else
    {
        setvbuf(stdin, NULL, _IONBF, 0);
    }

    // Redirect STDOUT if the console has an output handle
    if (GetStdHandle(STD_OUTPUT_HANDLE) != INVALID_HANDLE_VALUE)
    {
        if (freopen_s(&fp, "CONOUT$", "w", stdout) != 0)
            Result = false;
    }
    else
    {
        setvbuf(stdout, NULL, _IONBF, 0);
    }

    // Redirect STDERR if the console has an error handle
    if (GetStdHandle(STD_ERROR_HANDLE) != INVALID_HANDLE_VALUE)
    {
        if (freopen_s(&fp, "CONOUT$", "w", stderr) != 0)
            Result = false;
    }
    else
    {
        setvbuf(stderr, NULL, _IONBF, 0);
    }

    return Result;
}

// Sets up a standard stream (stdout or stderr).
fn_internal win32_standard_stream 
win32_get_standard_stream(DWORD StreamType)
{
    win32_standard_stream Result{};

    // If we don't have our own stream and can't find a parent console, allocate a new console.
    Result.Handle = GetStdHandle(StreamType);
    if (!Result.Handle || Result.Handle == INVALID_HANDLE_VALUE)
    {
        if (!AttachConsole(ATTACH_PARENT_PROCESS))
        {
            AllocConsole();
            bool redirect_Result = Win32RedirectConsoleIo();
            assert(redirect_Result);
        }

        Result.Handle = GetStdHandle(StreamType);
        assert(Result.Handle != INVALID_HANDLE_VALUE);
    }

    // Check if the stream is redirected to a file. If it does, check if the file already exists.
    if (Result.Handle != INVALID_HANDLE_VALUE)
    {
        DWORD dummy;
        DWORD type = GetFileType(Result.Handle) & (~FILE_TYPE_REMOTE);
        Result.IsRedirected = (type == FILE_TYPE_CHAR) ? !GetConsoleMode(Result.Handle, &dummy) : true;
    }

    return Result;
}

// Prints a message to a platform stream. If the stream is a console, uses supplied colors.
static void win32_print_to_stream(const char* message, s64 message_length, win32_standard_stream stream, WORD text_color)
{
    // If redirected, write to a file instead of console.
    DWORD dummy;
    if (stream.IsRedirected)
    {
        WriteFile(stream.Handle, message, (DWORD)message_length, &dummy, 0);
    }
    else
    {
        WORD previous_color = Win32GetConsoleColor(stream.Handle);

        SetConsoleTextAttribute(stream.Handle, text_color);
        WriteConsole(stream.Handle, message, (DWORD)message_length, &dummy, 0);

        // Restore console colors
        SetConsoleTextAttribute(stream.Handle, previous_color);
    }
}

void 
PlatformLogToConsole(bool IsError, log_color Foreground, log_color Background, const istr8& Message)
{
    Win32ConsoleColor Win32Foreground = LogColorToWin32Color(Foreground);
    Win32ConsoleColor Win32Background = LogColorToWin32Color(Background);
    WORD Win32ConsoleColor = Win32ComposeConsoleColor(Win32Foreground, Win32Background);

    if (IsError)
    {
        var_persist win32_standard_stream ErrorStream = win32_get_standard_stream(STD_ERROR_HANDLE);
        win32_print_to_stream(Message.Ptr(), Message.Length(), ErrorStream, Win32ConsoleColor);
    }
    else
    {
        var_persist win32_standard_stream stream = win32_get_standard_stream(STD_OUTPUT_HANDLE);
        win32_print_to_stream(Message.Ptr(), Message.Length(), stream, Win32ConsoleColor);
    }
}

void 
PlatformLogToDebugConsole(const istr8& Message)
{
#if _DEBUG
    if (IsDebuggerPresent())
    {
        OutputDebugStringA(Message.Ptr());
    }
#endif
}
