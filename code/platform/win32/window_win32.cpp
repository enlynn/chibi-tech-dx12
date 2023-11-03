#include "common_win32.h"

struct win32_window
{
	HWND      Handle   = nullptr;
	HINSTANCE Instance = nullptr;
};

fn_internal LRESULT CALLBACK
Win32WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	// Retrieve user data, if any
	platform_window* Window = nullptr;
	LONG_PTR GetWindowDataResult = GetWindowLongPtr(hwnd, GWLP_USERDATA);
	if (GetWindowDataResult == 0)
	{
		DWORD LastError = GetLastError();
		//Nothing for now...
	}
	else
	{
		Window = (platform_window*)GetWindowDataResult;
	}

	switch (message)
	{
		case WM_NCCREATE:
		{
			LPCREATESTRUCT CreateInfo = (LPCREATESTRUCT)lparam;
			if (CreateInfo->lpCreateParams != 0)
			{
				// SetWindowLongPtr does not properly set the last error if SetWindowLongPtr
				// has not been called previously, so it must be set manually
				SetLastError(ERROR_SUCCESS);

				LONG_PTR SetWindowDataResult = SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)CreateInfo->lpCreateParams);
				if (SetWindowDataResult == 0)
				{
					// If the GWL_USERDATA was not set previously, SetWindowLongPtr will return 0.
					// If the function was successful, then GetLastError will return ERROR_SUCCESS
					DWORD LastError = GetLastError();
					assert(LastError == ERROR_SUCCESS);
				}
			}
		} break;

		case WM_CLOSE:   // TODO(enlynn): Close popup instead of immediate close
		case WM_DESTROY: // TODO(enlynn): Reset program state?
		case WM_QUIT:
		{
			if (Window)
			{
				Window->SetIsRunning(false);
			}
		} break;

		case WM_SIZE: {
			if (Window)
			{
				Window->SetNeedsResized();
			}
		} break;
	}

	return DefWindowProc(hwnd, message, wparam, lparam);
}

fn_internal void
Win32RegisterWindowClass(const char* ClassName)
{
	WNDCLASSEX WindowClass = {};
	ZeroMemory(&WindowClass, sizeof(WNDCLASSEX));

	WindowClass.cbSize        = sizeof(WNDCLASSEX);
	WindowClass.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	WindowClass.lpfnWndProc   = Win32WindowProc;
	WindowClass.hInstance     = GetModuleHandle(nullptr);
	WindowClass.hCursor       = (HICON)LoadImage(0, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
	WindowClass.hIcon         = (HICON)LoadImage(0, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
	WindowClass.lpszClassName = ClassName;

	RegisterClassEx(&WindowClass);
}

fn_internal void 
Win32CreateWindow(win32_window* Window, 
	const char* ClassName, const char* WindowName, 
	u32 RequestedWidth, u32 RequestedHeight,
	void* UserData = nullptr)
{
	int ScreenWidth  = GetSystemMetrics(SM_CXSCREEN);
	int ScreenHeight = GetSystemMetrics(SM_CYSCREEN);

	RECT Rect = { 0, 0, (LONG)RequestedWidth, (LONG)RequestedHeight };
	AdjustWindowRect(&Rect, WS_OVERLAPPEDWINDOW, FALSE);

	LONG Width  = Rect.right - Rect.left;
	LONG Height = Rect.bottom - Rect.top;

	LONG WindowX = (ScreenWidth - Width) / 2;
	LONG WindowY = (ScreenHeight - Height) / 2;

	if (WindowX < 0) WindowX = CW_USEDEFAULT;
	if (WindowY < 0) WindowY = CW_USEDEFAULT;
	if (Width < 0)   Width   = CW_USEDEFAULT;
	if (Height < 0)  Height  = CW_USEDEFAULT;

	DWORD Style   = WS_OVERLAPPEDWINDOW;
	DWORD ExStyle = WS_EX_OVERLAPPEDWINDOW;

	Window->Handle = CreateWindowEx(
		ExStyle, ClassName, WindowName,         Style,
		WindowX, WindowY,   Width,              Height,
		0,       0,         GetModuleHandle(0), UserData);

	Window->Instance = GetModuleHandle(nullptr);
}

void
platform_window::Init(const char* Name, u32 Width, u32 Height)
{
	const char* WindowClassName = "chibitech_window";

	mWindowState.IsRunning = true;

	win32_window* Internal = new win32_window;
	Win32RegisterWindowClass(WindowClassName);
	Win32CreateWindow(Internal, WindowClassName, Name, Width, Height, (void*)this);

	{ // TODO: Update this on Monitor change
		HDC RefreshDc = GetDC(Internal->Handle);
		int MonitorRefreshHz = 60;
		int Win32RefreshRate = GetDeviceCaps(RefreshDc, VREFRESH);
		if (Win32RefreshRate > 1)
		{
			MonitorRefreshHz = Win32RefreshRate;
		}
		f32 GameUpdateHz = (f32)MonitorRefreshHz;
		mMonitorRefreshRate = 1.0 / (f64)(GameUpdateHz);
		ReleaseDC(Internal->Handle, RefreshDc);
	}

	ShowWindow(Internal->Handle, SW_SHOWNORMAL);
	mWindowState.IsShown = true;
	mInternalState       = Internal;
}

void
platform_window::Deinit()
{
	if (mInternalState)
	{
		delete mInternalState;
		mInternalState = nullptr;
	}
}

platform_window_handle 
platform_window::GetHandle() const
{
	assert(mInternalState);

	win32_window* Win32State = (win32_window*)mInternalState;
	return Win32State->Handle;
}

platform_window_instance 
platform_window::GetInstance() const
{
	assert(mInternalState);

	win32_window* Win32State = (win32_window*)mInternalState;
	return Win32State->Instance;
}

platform_window_rect 
platform_window::GetWindowRect() const
{
	assert(mInternalState);
	win32_window* Win32State = (win32_window*)mInternalState;

	RECT Rect;
	GetClientRect(Win32State->Handle, &Rect);

	platform_window_rect Result = {};
	Result.Width                = Rect.right - Rect.left;
	Result.Height               = Rect.bottom - Rect.top;
	Result.X                    = Rect.left;
	Result.Y                    = Rect.top;

	return Result;
}

platform_pump_message_result 
platform_window::PumpMessages()
{
	platform_pump_message_result Result = {};

	if (!IsRunning())
	{
		Result.Close = true;
		return Result;
	}
	
	MSG msg{};
	while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
	{
		switch (msg.message)
		{
			case WM_CLOSE:   // TODO(enlynn): Close popup instead of immediate close
			case WM_DESTROY: // TODO(enlynn): Reset program state?
			case WM_QUIT:
			{
				SetIsRunning(false);
				Result.Close = true;
			} break;

			case WM_SIZE: 
			{
				SetNeedsResized();
				Result.Resize = true;
			} break;

			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_KEYDOWN:
			case WM_KEYUP:
			{
			} break;

			case WM_LBUTTONDOWN:
			case WM_MBUTTONDOWN:
			case WM_RBUTTONDOWN:
			case WM_LBUTTONUP:
			case WM_MBUTTONUP:
			case WM_RBUTTONUP:
			{
			} break;

			case WM_MOUSEMOVE:
			{
			} break;

			case WM_MOUSEWHEEL:
			{
			} break;

			default:
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			} break;
		}
	}

	return Result;
}