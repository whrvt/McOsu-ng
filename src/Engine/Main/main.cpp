//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		cross-platform main() entrypoint (using SDL3 main callbacks)
//
// $NoKeywords: $sdlcallbacks $main
//===============================================================================//

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#include "Profiler.h"
#include "Timer.h"

#include "Engine.h"
#include "SDLEnvironment.h"

// global environment
static SDLEnvironment *g_env = nullptr;

ConVar fps_max("fps_max", 60.0f, FCVAR_NONE, "framerate limiter, foreground");
ConVar fps_max_yield("fps_max_yield", true, FCVAR_NONE, "always release rest of timeslice once per frame (call scheduler via sleep(0))");
ConVar fps_max_background("fps_max_background", 30.0f, FCVAR_NONE, "framerate limiter, background");
ConVar fps_unlimited("fps_unlimited", false, FCVAR_NONE);
ConVar fps_unlimited_yield("fps_unlimited_yield", true, FCVAR_NONE,
                           "always release rest of timeslice once per frame (call scheduler via sleep(0)), even if unlimited fps are enabled");
extern ConVar _fullscreen_;
extern ConVar _windowed_;
extern ConVar _fullscreen_windowed_borderless_;

// window configuration
static constexpr auto WINDOW_TITLE = "McEngine";
static constexpr auto WINDOW_WIDTH = 1280L;
static constexpr auto WINDOW_HEIGHT = 720L;
static constexpr auto WINDOW_WIDTH_MIN = 100;
static constexpr auto WINDOW_HEIGHT_MIN = 100;

// platform-specific headers
#if defined(MCENGINE_PLATFORM_WINDOWS) || (defined(_WIN32) && !defined(__linux__))

// #define MCENGINE_WINDOWS_REALTIMESTYLUS_SUPPORT
// #define MCENGINE_WINDOWS_TOUCH_SUPPORT
#ifdef MCENGINE_WINDOWS_TOUCH_SUPPORT
#define WINVER 0x0A00 // windows 10, to enable the ifdefs in winuser.h for touch
#endif
#ifdef MCENGINE_FEATURE_NETWORKING
#include <winsock2.h>
#endif
#include <dwmapi.h>
#include <libloaderapi.h>
#include <windows.h>

void initWin32ArgSettings()
{
	UString cmdline = GetCommandLine();

	// disable IME text input if -noime
	if (cmdline.findIgnoreCase("-noime"))
	{
		typedef BOOL(WINAPI * pfnImmDisableIME)(DWORD);
		HMODULE hImm32 = LoadLibrary(TEXT("imm32.dll"));
		if (hImm32 != NULL)
		{
			auto pImmDisableIME = (pfnImmDisableIME)GetProcAddress(hImm32, "ImmDisableIME");
			if (pImmDisableIME != NULL)
				pImmDisableIME(-1);
			FreeLibrary(hImm32);
		}
	}

	// enable DPI awareness if not -nodpi
	if (!cmdline.findIgnoreCase("-nodpi"))
	{
		typedef WINBOOL(WINAPI * PSPDA)(void);
		auto g_SetProcessDPIAware = (PSPDA)GetProcAddress(GetModuleHandle(TEXT("user32.dll")), "SetProcessDPIAware");
		if (g_SetProcessDPIAware != NULL)
			g_SetProcessDPIAware();
	}
}

#endif

// struct to hold application state
struct AppState
{
	Engine *engine;
	SDL_Window *window;
	SDL_GLContext context;

	Timer *frameTimer;
	Timer *deltaTimer;
	Timer *fpsCalcTimer;

	uint64_t frameCountSinceLastFpsCalc;
	double fpsAdjustment;

	bool minimized;
	bool hasFocus;
};

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
#if defined(MCENGINE_PLATFORM_WINDOWS) || (defined(_WIN32) && !defined(__linux__))
	initWin32ArgSettings();
#endif

	SDL_SetHint(SDL_HINT_VIDEO_DOUBLE_BUFFER, "1");
	constexpr auto flags = SDL_INIT_VIDEO;

	// initialize SDL
	if (!SDL_Init(flags))
	{
		debugLog("Couldn't SDL_Init(): %s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	// create environment
	g_env = new SDLEnvironment();

	// create application state
	auto *state = new AppState();
	*appstate = state;

	state->engine = nullptr;
	state->window = nullptr;
	state->context = nullptr;
	state->frameTimer = nullptr;
	state->deltaTimer = nullptr;
	state->fpsCalcTimer = nullptr;
	state->frameCountSinceLastFpsCalc = 0;
	state->fpsAdjustment = 0.0;
	state->minimized = false;
	state->hasFocus = true;

	// set up SDL logging
	SDL_SetLogOutputFunction(
	    [](void *, int category, SDL_LogPriority, const char *message) {
		    const char *catStr;
		    switch (category)
		    {
		    case SDL_LOG_CATEGORY_APPLICATION:
			    catStr = "APP";
			    break;
		    case SDL_LOG_CATEGORY_ERROR:
			    catStr = "ERR";
			    break;
		    case SDL_LOG_CATEGORY_SYSTEM:
			    catStr = "SYS";
			    break;
		    case SDL_LOG_CATEGORY_AUDIO:
			    catStr = "AUD";
			    break;
		    case SDL_LOG_CATEGORY_VIDEO:
			    catStr = "VID";
			    break;
		    case SDL_LOG_CATEGORY_RENDER:
			    catStr = "REN";
			    break;
		    case SDL_LOG_CATEGORY_INPUT:
			    catStr = "INP";
			    break;
		    case SDL_LOG_CATEGORY_CUSTOM:
			    catStr = "USR";
			    break;
		    default:
			    catStr = "???";
			    break;
		    }
		    fprintf(stderr, "SDL[%s]: %s\n", catStr, message);
	    },
	    nullptr);

	// pre window-creation settings
	if constexpr (Env::cfg((REND::GL | REND::GLES2 | REND::GLES32), !REND::DX11))
	{
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, Env::cfg(REND::GL) ? SDL_GL_CONTEXT_PROFILE_COMPATIBILITY : SDL_GL_CONTEXT_PROFILE_ES);
		if constexpr (!Env::cfg(REND::GL))
		{
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, Env::cfg(REND::GLES2) ? 2 : 3);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, Env::cfg(REND::GLES2) ? 0 : 2);
		}
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	}

	constexpr auto windowFlags = SDL_WINDOW_HIDDEN | SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS |
	                             ((Env::cfg((REND::GL | REND::GLES2 | REND::GLES32), !REND::DX11)) ? SDL_WINDOW_OPENGL
	                              : (Env::cfg(REND::VK, !REND::DX11))                              ? SDL_WINDOW_VULKAN
	                                                                                               : 0UL);

	SDL_PropertiesID props = SDL_CreateProperties();
	SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, WINDOW_TITLE);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_CENTERED);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_CENTERED);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, WINDOW_WIDTH);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, WINDOW_HEIGHT);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, false);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_MAXIMIZED_BOOLEAN, false);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_FLAGS_NUMBER, windowFlags);

	const bool shouldBeBorderless = _fullscreen_windowed_borderless_.getBool();
	const bool shouldBeFullscreen = _fullscreen_.getBool() || !_windowed_.getBool() || shouldBeBorderless;

	if (shouldBeBorderless)
		SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_BORDERLESS_BOOLEAN, true);
	else if (shouldBeFullscreen)
		SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_FULLSCREEN_BOOLEAN, true);

	if constexpr (Env::cfg(OS::WINDOWS))
		SDL_SetHintWithPriority(SDL_HINT_WINDOWS_RAW_KEYBOARD, "1", SDL_HINT_OVERRIDE);

	SDL_SetHintWithPriority(SDL_HINT_MOUSE_RELATIVE_MODE_CENTER, "0", SDL_HINT_OVERRIDE);
	SDL_SetHintWithPriority(SDL_HINT_TOUCH_MOUSE_EVENTS, "0", SDL_HINT_OVERRIDE);
	SDL_SetHintWithPriority(SDL_HINT_MOUSE_EMULATE_WARP_WITH_RELATIVE, "0", SDL_HINT_OVERRIDE);

	// create window
	state->window = SDL_CreateWindowWithProperties(props);
	SDL_DestroyProperties(props);

	if (state->window == NULL)
	{
		debugLog("Couldn't SDL_CreateWindow(): %s\n", SDL_GetError());
		delete state;
		return SDL_APP_FAILURE;
	}

	// store the window in the SDLEnvironment
	g_env->m_window = state->window;

	// get the screen refresh rate, and set fps_max to that as default
	{
		const SDL_DisplayMode *currentDisplayMode;
		const SDL_DisplayID display = SDL_GetDisplayForWindow(state->window);
		currentDisplayMode = SDL_GetCurrentDisplayMode(display);

		if (currentDisplayMode && currentDisplayMode->refresh_rate > 0)
		{
			debugLog("Display %d refresh rate is %f Hz, setting default fps_max to %f.\n", display, currentDisplayMode->refresh_rate, currentDisplayMode->refresh_rate);
			fps_max.setValue(currentDisplayMode->refresh_rate);
			fps_max.setDefaultFloat(currentDisplayMode->refresh_rate);
		}
		else
		{
			debugLog("Couldn't SDL_GetCurrentDisplayMode(SDL display: %d): %s\n", display, SDL_GetError());
		}
	}

	// create OpenGL context
	if constexpr (Env::cfg((REND::GL | REND::GLES2 | REND::GLES32), !REND::DX11))
	{
		state->context = SDL_GL_CreateContext(state->window);
		SDL_GL_MakeCurrent(state->window, state->context);
	}

	// disable unused events
	// joystick
	SDL_SetEventEnabled(SDL_EVENT_JOYSTICK_AXIS_MOTION, false);
	SDL_SetEventEnabled(SDL_EVENT_JOYSTICK_BALL_MOTION, false);
	SDL_SetEventEnabled(SDL_EVENT_JOYSTICK_HAT_MOTION, false);
	SDL_SetEventEnabled(SDL_EVENT_JOYSTICK_BUTTON_DOWN, false);
	SDL_SetEventEnabled(SDL_EVENT_JOYSTICK_BUTTON_UP, false);
	SDL_SetEventEnabled(SDL_EVENT_JOYSTICK_ADDED, false);
	SDL_SetEventEnabled(SDL_EVENT_JOYSTICK_REMOVED, false);
	SDL_SetEventEnabled(SDL_EVENT_JOYSTICK_BATTERY_UPDATED, false);
	SDL_SetEventEnabled(SDL_EVENT_JOYSTICK_UPDATE_COMPLETE, false);

	// pen
	SDL_SetEventEnabled(SDL_EVENT_PEN_PROXIMITY_IN, false);
	SDL_SetEventEnabled(SDL_EVENT_PEN_PROXIMITY_OUT, false);
	SDL_SetEventEnabled(SDL_EVENT_PEN_DOWN, false);
	SDL_SetEventEnabled(SDL_EVENT_PEN_UP, false);
	SDL_SetEventEnabled(SDL_EVENT_PEN_BUTTON_DOWN, false);
	SDL_SetEventEnabled(SDL_EVENT_PEN_BUTTON_UP, false);
	SDL_SetEventEnabled(SDL_EVENT_PEN_MOTION, false);
	SDL_SetEventEnabled(SDL_EVENT_PEN_AXIS, false);

	// touch
	SDL_SetEventEnabled(SDL_EVENT_FINGER_DOWN, false);
	SDL_SetEventEnabled(SDL_EVENT_FINGER_UP, false);
	SDL_SetEventEnabled(SDL_EVENT_FINGER_MOTION, false);
	SDL_SetEventEnabled(SDL_EVENT_FINGER_CANCELED, false);

	// create timers
	state->frameTimer = new Timer();
	state->deltaTimer = new Timer();
	state->fpsCalcTimer = new Timer();

	SDL_SetWindowMinimumSize(state->window, WINDOW_WIDTH_MIN, WINDOW_HEIGHT_MIN);

	// initialize mouse position
	{
		float x, y;
		SDL_GetGlobalMouseState(&x, &y);
		g_env->m_vLastAbsMousePos.x = x;
		g_env->m_vLastAbsMousePos.y = y;
	}

	// initialize engine
	state->engine = new Engine(g_env, argc > 1 ? argv[1] : ""); // tODO: proper arg support
	g_env->m_engine = state->engine;

	// make the window visible
	SDL_ShowWindow(state->window);
	SDL_RaiseWindow(state->window);

	state->engine->loadApp();

	state->frameTimer->update();
	state->deltaTimer->update();
	state->fpsCalcTimer->update();

	// SDL3 stops listening to text input globally when window is created
	SDL_StartTextInput(state->window);
	SDL_SetWindowKeyboardGrab(state->window, false); // this allows windows key and such to work

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
	VPROF_MAIN();

	auto *state = static_cast<AppState *>(appstate);

	if (!g_env->m_bRunning)
	{
		return SDL_APP_SUCCESS;
	}

	// update
	{
		state->deltaTimer->update();
		state->engine->setFrameTime(state->deltaTimer->getDelta());
		state->engine->onUpdate();
	}

	// draw
	{
		g_env->m_bDrawing = true;
		state->engine->onPaint();
		g_env->m_bDrawing = false;
	}

	// delay the next frame (if desired)
	{
		VPROF_BUDGET("FPSLimiter", VPROF_BUDGETGROUP_SLEEP);

		const bool inBackground = state->minimized || !state->hasFocus;
		const bool shouldSleep = inBackground || (!fps_unlimited.getBool() && fps_max.getFloat() > 0);
		const bool shouldYield = fps_unlimited_yield.getBool() || fps_max_yield.getBool();

		if (shouldSleep)
		{
			const float targetFps = inBackground ? fps_max_background.getFloat() : fps_max.getFloat();

			state->frameCountSinceLastFpsCalc++;
			state->fpsCalcTimer->update();

			if (state->fpsCalcTimer->getElapsedTime() >= 0.5f)
			{
				if (!inBackground)
				{
					const double actualGameFps = static_cast<double>(state->frameCountSinceLastFpsCalc) / state->fpsCalcTimer->getElapsedTime();
					if (actualGameFps < targetFps * 0.99f && actualGameFps > targetFps * 0.85f)
						state->fpsAdjustment -= 0.5f;
					else if (actualGameFps > targetFps * 1.005f)
						state->fpsAdjustment += 0.5f;
					state->fpsAdjustment = clamp<double>(state->fpsAdjustment, -15.0f, 0.0f);
				}
				else
					state->fpsAdjustment = 0.0f;

				// reset fps adjustment timer for the next measurement period
				state->frameCountSinceLastFpsCalc = 0;
				state->fpsCalcTimer->start();
			}
			state->frameTimer->update();
			const double frameTimeDelta = state->frameTimer->getDelta();
			const double adjustedTargetFrameTime = (1.0f / targetFps) * (1.0f + (inBackground ? 0.0f : state->fpsAdjustment) / 100.0f);

			// finally sleep for the adjusted amount of time
			if (frameTimeDelta < adjustedTargetFrameTime)
				g_env->sleep(static_cast<unsigned int>((adjustedTargetFrameTime - frameTimeDelta) * SDL_US_PER_SECOND));
			else if (shouldYield)
				g_env->sleep(0);

			// set the start time of the next loop iteration
			state->frameTimer->update();
		}
		else if (shouldYield)
			g_env->sleep(0);
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
	auto *state = static_cast<AppState *>(appstate);

	VPROF_BUDGET("SDL", VPROF_BUDGETGROUP_WNDPROC);

	switch (event->type)
	{
	case SDL_EVENT_QUIT:
		if (g_env->m_bRunning)
		{
			g_env->m_bRunning = false;
			state->engine->onShutdown();
		}
		return SDL_APP_SUCCESS;

	// window events
	case SDL_EVENT_WINDOW_FIRST ... SDL_EVENT_WINDOW_LAST:
		switch (event->window.type)
		{
		case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
			if (g_env->m_bRunning)
			{
				g_env->m_bRunning = false;
				state->engine->onShutdown();
			}
			return SDL_APP_SUCCESS;

		case SDL_EVENT_WINDOW_FOCUS_GAINED:
			state->hasFocus = true;
			g_env->m_bHasFocus = true;
			state->engine->onFocusGained();
			break;

		case SDL_EVENT_WINDOW_FOCUS_LOST:
			state->hasFocus = false;
			g_env->m_bHasFocus = false;
			state->engine->onFocusLost();
			break;

		case SDL_EVENT_WINDOW_MAXIMIZED:
			state->minimized = false;
			g_env->m_bMinimized = false;
			state->engine->onMaximized();
			break;

		case SDL_EVENT_WINDOW_MINIMIZED:
			state->minimized = true;
			g_env->m_bMinimized = true;
			state->engine->onMinimized();
			break;

		case SDL_EVENT_WINDOW_RESTORED:
			state->minimized = false;
			g_env->m_bMinimized = false;
			state->engine->onRestored();
			break;

		case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
		case SDL_EVENT_WINDOW_RESIZED:
			state->engine->requestResolutionChange(Vector2(static_cast<float>(event->window.data1), static_cast<float>(event->window.data2)));
			break;
		default:
			break;
		}
		break;

	// keyboard events
	case SDL_EVENT_KEY_DOWN:
		state->engine->onKeyboardKeyDown(event->key.scancode);
		break;

	case SDL_EVENT_KEY_UP:
		state->engine->onKeyboardKeyUp(event->key.scancode);
		break;

	case SDL_EVENT_TEXT_INPUT: {
		UString nullTerminatedTextInputString(event->text.text);
		for (int i = 0; i < nullTerminatedTextInputString.length(); i++)
		{
			state->engine->onKeyboardChar((KEYCODE)nullTerminatedTextInputString[i]);
		}
	}
	break;

	// mouse events
	case SDL_EVENT_MOUSE_BUTTON_DOWN:
		state->engine->onMouseButtonChange(event->button.button, true);
		break;

	case SDL_EVENT_MOUSE_BUTTON_UP:
		state->engine->onMouseButtonChange(event->button.button, false);
		break;

	case SDL_EVENT_MOUSE_WHEEL:
		if (event->wheel.x != 0)
			state->engine->onMouseWheelHorizontal(event->wheel.x > 0 ? 120 * std::abs(static_cast<int>(event->wheel.x)) : -120 * std::abs(static_cast<int>(event->wheel.x)));
		if (event->wheel.y != 0)
			state->engine->onMouseWheelVertical(event->wheel.y > 0 ? 120 * std::abs(static_cast<int>(event->wheel.y)) : -120 * std::abs(static_cast<int>(event->wheel.y)));
		break;

	case SDL_EVENT_MOUSE_MOTION:
		// cache the position
		g_env->m_vLastRelMousePos.x = event->motion.xrel;
		g_env->m_vLastRelMousePos.y = event->motion.yrel;
		g_env->m_vLastAbsMousePos.x = event->motion.x;
		g_env->m_vLastAbsMousePos.y = event->motion.y;
		state->engine->onMouseMotion(event->motion.x, event->motion.y, event->motion.xrel, event->motion.yrel, event->motion.which != 0);
		break;
	default:
		break;
	}

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult)
{
	auto *state = static_cast<AppState *>(appstate);

	// cleanup timers
	SAFE_DELETE(state->frameTimer);
	SAFE_DELETE(state->deltaTimer);
	SAFE_DELETE(state->fpsCalcTimer);

	// cleanup gl context
	if constexpr (Env::cfg((REND::GL | REND::GLES2 | REND::GLES32), !REND::DX11))
		SDL_GL_DestroyContext(state->context);

	SDL_StopTextInput(state->window);

	// cleanup engine
	SAFE_DELETE(state->engine);

	// cleanup env
	SAFE_DELETE(g_env);

	// cleanup appstate
	SAFE_DELETE(state);

	// window is destroyed by SDL_Quit which is called after this function
}
