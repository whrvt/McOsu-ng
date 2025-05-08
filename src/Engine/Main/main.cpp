//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		Cross-platform main() entrypoint
//
// $NoKeywords: $main
//===============================================================================//

#include "SDLEnvironment.h"

#if defined(MCENGINE_PLATFORM_WINDOWS) || (defined(_WIN32) && !defined(__linux__)) // mingw, who knows

//#define MCENGINE_WINDOWS_REALTIMESTYLUS_SUPPORT
//#define MCENGINE_WINDOWS_TOUCH_SUPPORT
#ifdef MCENGINE_WINDOWS_TOUCH_SUPPORT
# define WINVER 0x0A00 // Windows 10, to enable the ifdefs in winuser.h for touch
#endif
#ifdef MCENGINE_FEATURE_NETWORKING
# include <winsock2.h>
#endif
#include <windows.h>
#include <dwmapi.h>
#include <libloaderapi.h>
// TODO: fix this strange argument parsing
#ifdef UNICODE
#define MAIN int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
#else
#define MAIN int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
#endif

// TODO: handle apple-specific shit (not that i can really test that easily...)
#elif defined(MCENGINE_PLATFORM_LINUX) || defined(__APPLE__) || defined(__EMSCRIPTEN__) || defined(MCENGINE_PLATFORM_WASM)
#define MAIN int main(int argc, char *argv[])
#else
#error "No entrypoint is defined for this platform"
#endif

MAIN
{
#if defined(MCENGINE_PLATFORM_WINDOWS) || (defined(_WIN32) && !defined(__linux__))
	// disable IME text input
#ifdef UNICODE
	if (wcsstr(pCmdLine, TEXT("-noime")) != NULL)
#else
	if (strstr(pCmdLine, TEXT("-noime")) != NULL)
#endif
	{
		typedef BOOL (WINAPI *pfnImmDisableIME)(DWORD);
		HMODULE hImm32 = LoadLibrary(TEXT("imm32.dll"));
		if (hImm32 != NULL)
		{
			pfnImmDisableIME pImmDisableIME = (pfnImmDisableIME)GetProcAddress(hImm32, "ImmDisableIME");
			if (pImmDisableIME == NULL)
				FreeLibrary(hImm32);
			else
			{
				pImmDisableIME(-1);
				FreeLibrary(hImm32);
			}
		}
	}

	// if supported (>= Windows Vista), enable DPI awareness so that GetSystemMetrics returns correct values
	// without this, on e.g. 150% scaling, the screen pixels of a 1080p monitor would be reported by GetSystemMetrics(SM_CXSCREEN/SM_CYSCREEN) as only 720p!
#ifdef UNICODE
	if (wcsstr(pCmdLine, TEXT("-nodpi")) == NULL)
#else
	if (strstr(pCmdLine, TEXT("-nodpi")) == NULL)
#endif
	{
		typedef WINBOOL (WINAPI *PSPDA)(void);
		PSPDA g_SetProcessDPIAware = (PSPDA)GetProcAddress(GetModuleHandle(TEXT("user32.dll")), "SetProcessDPIAware");
		if (g_SetProcessDPIAware != NULL)
			g_SetProcessDPIAware();
	}

	// build "fake" argc + argv (whyyy)
	const int argc = 2;
	char *argv[argc];
	char arg0[] = PACKAGE_NAME;
	argv[0] = arg0;

#ifdef UNICODE
	size_t mbssize = std::wcstombs(NULL, pCmdLine, 0) + 1;
	argv[1] = (char*)malloc(mbssize);
	wcstombs(argv[1], pCmdLine, mbssize);
#else
	argv[1] = pCmdLine;
#endif
#endif
	return SDLEnvironment().main(argc, argv);
}
