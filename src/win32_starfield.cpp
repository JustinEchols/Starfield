#include <windows.h>
#include <stdint.h>
#include <stdlib.h>

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef s32 b32;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

#define global_variable static
#define local_persist static
#define internal static
#define STAR_COUNT 4096

struct star
{
	f32 x, y, z;
};

struct stars
{
	star Star[STAR_COUNT];
};

struct win32_back_buffer
{
	BITMAPINFO Info;
	void *memory;
	int bytes_per_pixel;
	int stride;
	int width;
	int height;

};

global_variable b32 GLOBAL_RUNNING;
global_variable win32_back_buffer Win32BackBuffer;
global_variable LARGE_INTEGER tick_frequency;

internal LRESULT CALLBACK
win32_main_window_callback(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	switch (Message) {
		case WM_CLOSE:
		case WM_DESTROY:
		{
			GLOBAL_RUNNING = false;
		} break;
		case WM_SIZE:
		{
			RECT Rect;
			GetClientRect(Window, &Rect);

			int client_width = Rect.right - Rect.left;
			int client_height = Rect.bottom - Rect.top;

			Win32BackBuffer.width = client_width;
			Win32BackBuffer.height = client_height;
			Win32BackBuffer.stride = Win32BackBuffer.width * Win32BackBuffer.bytes_per_pixel;

			Win32BackBuffer.Info.bmiHeader.biSize = sizeof(Win32BackBuffer.Info.bmiHeader);
			Win32BackBuffer.Info.bmiHeader.biWidth = Win32BackBuffer.width;
			Win32BackBuffer.Info.bmiHeader.biHeight = Win32BackBuffer.height;
			Win32BackBuffer.Info.bmiHeader.biPlanes = 1;
			Win32BackBuffer.Info.bmiHeader.biBitCount = 32;
			Win32BackBuffer.Info.bmiHeader.biCompression = BI_RGB;

			if (Win32BackBuffer.memory) {
				VirtualFree(Win32BackBuffer.memory, 0, MEM_RELEASE);
			}

			int Win32BackBuffersize = Win32BackBuffer.width * Win32BackBuffer.height * Win32BackBuffer.bytes_per_pixel;
			Win32BackBuffer.memory = VirtualAlloc(0, Win32BackBuffersize, (MEM_COMMIT | MEM_RESERVE), PAGE_READWRITE);

		} break;
		case WM_PAINT:
		{
			PAINTSTRUCT PaintStruct;
			HDC DeviceContext = BeginPaint(Window, &PaintStruct);

			StretchDIBits(DeviceContext,
					0, 0, Win32BackBuffer.width, Win32BackBuffer.height,
					0, 0, Win32BackBuffer.width, Win32BackBuffer.height,
					Win32BackBuffer.memory,
					&Win32BackBuffer.Info,
					DIB_RGB_COLORS,
					SRCCOPY);

			EndPaint(Window, &PaintStruct);

		} break;
		default:
		{
			result = DefWindowProcA(Window, Message, wParam, lParam);
		}
	}
	return(result);
}

int CALLBACK
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	WNDCLASSA WindowClass = {};

	WindowClass.style = (CS_HREDRAW | CS_VREDRAW);
	WindowClass.lpfnWndProc = win32_main_window_callback;
	WindowClass.hInstance = hInstance;
	WindowClass.lpszClassName = "Main window";


	Win32BackBuffer = {};
	Win32BackBuffer.width = 960;
	Win32BackBuffer.height = 540;
	Win32BackBuffer.bytes_per_pixel = 4;
	Win32BackBuffer.stride = Win32BackBuffer.width * Win32BackBuffer.bytes_per_pixel;

	Win32BackBuffer.Info.bmiHeader.biSize = sizeof(Win32BackBuffer.Info.bmiHeader);
	Win32BackBuffer.Info.bmiHeader.biWidth = Win32BackBuffer.width;
	Win32BackBuffer.Info.bmiHeader.biHeight = Win32BackBuffer.height;
	Win32BackBuffer.Info.bmiHeader.biPlanes = 1;
	Win32BackBuffer.Info.bmiHeader.biBitCount = 32;
	Win32BackBuffer.Info.bmiHeader.biCompression = BI_RGB;

	QueryPerformanceFrequency(&tick_frequency);
	f32 ticks_per_second = (f32)tick_frequency.QuadPart;
	f32 time_for_each_tick = 1.0f / ticks_per_second;

	f32 star_spread = 64.0f;
	f32 star_speed = 10.0f;

	stars Stars;
	
	srand(2023);
	for (u32 i = 0; i < STAR_COUNT; i++) {
		Stars.Star[i].x = (((f32)rand() / (f32)RAND_MAX) * 2.0f + -1.0f);
		Stars.Star[i].y = (((f32)rand() / (f32)RAND_MAX) * 2.0f + -1.0f);
		Stars.Star[i].z = (((f32)rand() / (f32)RAND_MAX) * 1.0f + -0.00001f);

		Stars.Star[i].x *= star_spread;
		Stars.Star[i].y *= star_spread;
		Stars.Star[i].z *= star_spread;
	}

	if (RegisterClassA(&WindowClass)) {
		HWND Window = CreateWindowExA(
				0,
				WindowClass.lpszClassName,
				"Starfield",
				(WS_OVERLAPPEDWINDOW | WS_VISIBLE),
				CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
				0,
				0,
				hInstance,
				0);

		if (Window) {
			HDC DeviceContext = GetDC(Window);

			GLOBAL_RUNNING = true;

			f32 time_delta = 0.0f;
			LARGE_INTEGER tick_count_before;
			QueryPerformanceCounter(&tick_count_before);
			while (GLOBAL_RUNNING) {
				MSG Message;
				while (PeekMessage(&Message, Window, 0, 0, PM_REMOVE)) {
					TranslateMessage(&Message);
					DispatchMessage(&Message);
				}

				// Clear screen to black
				u32 *pixel = (u32 *)Win32BackBuffer.memory;
				for (s32 y = 0; y < Win32BackBuffer.height; y++) {
					for (s32 x = 0; x < Win32BackBuffer.width; x++) {
						// AA RR GG BB
						*pixel++ = 0;
					}
				}

				// Simulate star movement in z 
				for (u32 i = 0; i < STAR_COUNT; i++) {
					Stars.Star[i].z -= time_delta * star_speed;
					if (Stars.Star[i].z <= 0) {
						Stars.Star[i].x = ((((f32)rand() / (f32)RAND_MAX) * 2.0f) - 1.0f);
						Stars.Star[i].y = ((((f32)rand() / (f32)RAND_MAX) * 2.0f) - 1.0f);
						Stars.Star[i].z = ((((f32)rand() / (f32)RAND_MAX) * 1.0f) - 0.00001f);

						Stars.Star[i].x *= star_spread;
						Stars.Star[i].y *= star_spread;
						Stars.Star[i].z *= star_spread;
					}

					int star_x = (int)((Stars.Star[i].x / Stars.Star[i].z) * (Win32BackBuffer.width / 2) + (Win32BackBuffer.width / 2));
					int star_y = (int)((Stars.Star[i].y / Stars.Star[i].z) * (Win32BackBuffer.height / 2) + (Win32BackBuffer.height / 2));

					if ((star_x < 0) || (star_x >= Win32BackBuffer.width) ||
						(star_y < 0) || (star_y >= Win32BackBuffer.height)) {

						Stars.Star[i].x = ((((f32)rand() / (f32)RAND_MAX) * 2.0f) - 1.0f);
						Stars.Star[i].y = ((((f32)rand() / (f32)RAND_MAX) * 2.0f) - 1.0f);
						Stars.Star[i].z = ((((f32)rand() / (f32)RAND_MAX) * 1.0f) - 0.00001f);

						Stars.Star[i].x *= star_spread;
						Stars.Star[i].y *= star_spread;
						Stars.Star[i].z *= star_spread;
					} else {
						// Set pixel value
						u8 *pixel = ((u8 *)Win32BackBuffer.memory + Win32BackBuffer.stride * star_y + Win32BackBuffer.bytes_per_pixel * star_x);
						u32 *star = (u32 *)pixel;
						*star = 0xFFFFFFFF;
					}
				}

				// Blit
				StretchDIBits(DeviceContext,
						0, 0, Win32BackBuffer.width, Win32BackBuffer.height,
						0, 0, Win32BackBuffer.width, Win32BackBuffer.height,
						Win32BackBuffer.memory,
						&Win32BackBuffer.Info,
						DIB_RGB_COLORS,
						SRCCOPY);

				LARGE_INTEGER tick_count_after;
				QueryPerformanceCounter(&tick_count_after);

				f32 tick_count_elapsed = (f32)(tick_count_after.QuadPart - tick_count_before.QuadPart);
				time_delta = tick_count_elapsed * time_for_each_tick;
				tick_count_before = tick_count_after;
			}
		}
	}
	return(0);
}

