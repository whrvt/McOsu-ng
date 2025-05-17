//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		cross-platform main() entrypoint
//
// $NoKeywords: $sdlcallbacks $main
//===============================================================================//

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
#endif

#if defined(MCENGINE_PLATFORM_WASM) || defined(MCENGINE_FEATURE_MAINCALLBACKS)
#define MAIN_FUNC SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
#define nocbinline
#define SDL_MAIN_USE_CALLBACKS // this enables the use of SDL_AppInit/AppEvent/AppIterate instead of a traditional mainloop, needed for wasm
                               // (works on desktop too, but it's not necessary)
#else
#define MAIN_FUNC int main(int argc, char *argv[])
#define nocbinline static forceinline
#endif

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "Engine.h"
#include "Environment.h"
#include "Profiler.h"
#include "Timing.h"

// thin environment subclass to provide SDL callbacks with direct access to members
class SDLMain : public Environment
{
public:
	SDLMain(int argc, char *argv[]);
	~SDLMain();

	SDL_AppResult initialize(int argc, char *argv[]);
	SDL_AppResult iterate();
	SDL_AppResult handleEvent(SDL_Event *event);
	void shutdown() { Environment::shutdown(); }
	void shutdown(SDL_AppResult result);

private:
	// window and context
	SDL_GLContext m_context;

	// engine update timer
	Timer *m_deltaTimer;

	// when the next frame should render (for non-callback fps limiting)
	uint64_t m_iNextFrameTime;

	int m_iFpsMax;
	int m_iFpsMaxBG;

	// set iteration rate for callbacks
	// clang-format off
	inline void setFgFPS() { if constexpr (Env::cfg(FEAT::MAINCB)) SDL_SetHint(SDL_HINT_MAIN_CALLBACK_RATE, std::format("%i", m_iFpsMax).c_str()); else m_iNextFrameTime = SDL_GetTicksNS(); }
	inline void setBgFPS() { if constexpr (Env::cfg(FEAT::MAINCB)) SDL_SetHint(SDL_HINT_MAIN_CALLBACK_RATE, std::format("%i", m_iFpsMaxBG).c_str()); }
	// clang-format on

	// init methods
	void setupLogging();
	bool createWindow(int width, int height);
	void setupOpenGL();
	void configureEvents();

	// callback handlers
	void fps_max_callback(UString oldVal, UString newVal);
	void fps_max_background_callback(UString oldVal, UString newVal);
	void fps_unlimited_callback(UString oldVal, UString newVal);

	void parseArgs();
};

//*********************************//
//	SDL CALLBACKS/MAINLOOP BEGINS  //
//*********************************//

// called when the SDL_APP_SUCCESS (normal exit) or SDL_APP_FAILURE (something bad happened) event is returned from Init/Iterate/Event
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
	if (result == SDL_APP_FAILURE)
	{
		fprintf(stderr, "Force exiting now, a fatal error occurred. (SDL error: %s)\n", SDL_GetError());
		std::abort();
	}

	auto *fmain = static_cast<SDLMain *>(appstate);
	fmain->shutdown(result);
	SAFE_DELETE(fmain);

	if constexpr (!Env::cfg(FEAT::MAINCB))
		std::exit(0);
}

// (event queue processing) serialized with SDL_AppIterate
nocbinline SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
	return static_cast<SDLMain *>(appstate)->handleEvent(event);
}

// (update tick) serialized with SDL_AppEvent
nocbinline SDL_AppResult SDL_AppIterate(void *appstate)
{
	return static_cast<SDLMain *>(appstate)->iterate();
}

// actual main/init, called once
MAIN_FUNC /* int argc, char *argv[] */
{
	SDL_SetHint(SDL_HINT_VIDEO_DOUBLE_BUFFER, "1");
	if (!SDL_Init(SDL_INIT_VIDEO)) // other subsystems can be init later
	{
		fprintf(stderr, "Couldn't SDL_Init(): %s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	auto *fmain = new SDLMain(argc, argv);

#if !(defined(MCENGINE_PLATFORM_WASM) || defined(MCENGINE_FEATURE_MAINCALLBACKS))
	if (fmain->initialize(argc, argv) == SDL_APP_FAILURE)
		SDL_AppQuit(fmain, SDL_APP_FAILURE);

	constexpr int SIZE_EVENTS = 64;
	std::array<SDL_Event, SIZE_EVENTS> events;

	while (fmain->isRunning())
	{
		VPROF_MAIN();
		{
			// event collection
			VPROF_BUDGET("SDL", VPROF_BUDGETGROUP_WNDPROC);
			int eventCount = 0;

			SDL_PumpEvents();
			do
			{
				eventCount = SDL_PeepEvents(&events[0], SIZE_EVENTS, SDL_GETEVENT, SDL_EVENT_FIRST, SDL_EVENT_LAST);
				for (int i = 0; i < eventCount; ++i)
					SDL_AppEvent(fmain, &events[i]);
			} while (eventCount == SIZE_EVENTS);
		}
		{
			// engine update + draw + fps limiter
			SDL_AppIterate(fmain);
		}
	}

	return 0;
#else
	*appstate = fmain;
	return fmain->initialize(argc, argv);
#endif // SDL_MAIN_USE_CALLBACKS
}

//*******************************//
//	SDL CALLBACKS/MAINLOOP ENDS  //
//*******************************//

// window configuration
static constexpr auto WINDOW_TITLE = "McEngine";
static constexpr auto WINDOW_WIDTH = 1280L;
static constexpr auto WINDOW_HEIGHT = 720L;
static constexpr auto WINDOW_WIDTH_MIN = 100;
static constexpr auto WINDOW_HEIGHT_MIN = 100;

// convars
ConVar fps_max("fps_max", 420.0f, FCVAR_NONE, "framerate limiter, foreground");
ConVar fps_max_background("fps_max_background", 30.0f, FCVAR_NONE, "framerate limiter, background");
ConVar fps_unlimited("fps_unlimited", false, FCVAR_NONE);

// engine convars
extern ConVar _fullscreen_;
extern ConVar _windowed_;
extern ConVar _fullscreen_windowed_borderless_;

SDLMain::SDLMain(int, char *[]) : Environment()
{
	m_context = nullptr;
	m_deltaTimer = nullptr;

	m_iFpsMax = 420;
	m_iFpsMaxBG = 30;

	// setup callbacks
	fps_max.setCallback(fastdelegate::MakeDelegate(this, &SDLMain::fps_max_callback));
	fps_max_background.setCallback(fastdelegate::MakeDelegate(this, &SDLMain::fps_max_background_callback));
	fps_unlimited.setCallback(fastdelegate::MakeDelegate(this, &SDLMain::fps_unlimited_callback));
}

SDLMain::~SDLMain()
{
	// clean up timers
	SAFE_DELETE(m_deltaTimer);

	// clean up GL context
	if (m_context && (Env::cfg((REND::GL | REND::GLES2 | REND::GLES32 | REND::GL3), !REND::DX11)))
		SDL_GL_DestroyContext(m_context);

	// engine is deleted by parent (Environment) destructor
}

SDL_AppResult SDLMain::initialize(int argc, char *argv[])
{
	parseArgs();
	setupLogging();

	// create window with props
	if (!createWindow(WINDOW_WIDTH, WINDOW_HEIGHT))
		return SDL_APP_FAILURE;

	// create and make gl context current
	setupOpenGL();
	configureEvents();
	SDL_SetWindowMinimumSize(m_window, WINDOW_WIDTH_MIN, WINDOW_HEIGHT_MIN);

	// initialize mouse position
	{
		float x, y;
		SDL_GetGlobalMouseState(&x, &y);
		m_vLastAbsMousePos.x = x;
		m_vLastAbsMousePos.y = y;
	}

	// initialize engine
	m_engine = new Engine(this, argc > 1 ? argv[1] : "");

	// make window visible
	SDL_ShowWindow(m_window);
	SDL_RaiseWindow(m_window);

	// load app
	m_engine->loadApp();

	// start engine frame timer
	m_deltaTimer = new Timer();

	// get the screen refresh rate, and set fps_max to that as default
	if constexpr (!Env::cfg(OS::WASM)) // not in WASM
	{
		const SDL_DisplayID display = SDL_GetDisplayForWindow(m_window);
		const SDL_DisplayMode *currentDisplayMode = SDL_GetCurrentDisplayMode(display);

		if (currentDisplayMode && currentDisplayMode->refresh_rate > 0)
		{
			auto fourxhz = currentDisplayMode->refresh_rate * 4;
			if (fps_max.getFloat() == fps_max.getDefaultFloat())
			{
				debugLog("Display %d refresh rate is %f Hz, setting default fps_max to %f.\n", display, currentDisplayMode->refresh_rate, fourxhz);
				fps_max.setValue(fourxhz);
				fps_max.setDefaultFloat(fourxhz);
			}
		}
		else
		{
			debugLog("Couldn't SDL_GetCurrentDisplayMode(SDL display: %d): %s\n", display, SDL_GetError());
		}
	}

	// SDL3 stops listening to text input globally when window is created
	SDL_StartTextInput(m_window);
	SDL_SetWindowKeyboardGrab(m_window, false); // this allows windows key and such to work

	m_iNextFrameTime = SDL_GetTicksNS(); // init fps timer

	// return init success
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDLMain::handleEvent(SDL_Event *event)
{
	switch (event->type)
	{
	case SDL_EVENT_QUIT:
		if (m_bRunning)
		{
			m_bRunning = false;
			m_engine->onShutdown();
		}
		if constexpr (Env::cfg(FEAT::MAINCB))
			return SDL_APP_SUCCESS;
		else
			SDL_AppQuit(this, SDL_APP_SUCCESS);

	// window events
	case SDL_EVENT_WINDOW_FIRST ... SDL_EVENT_WINDOW_LAST:
		switch (event->window.type)
		{
		case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
			if (m_bRunning)
			{
				m_bRunning = false;
				m_engine->onShutdown();
			}
			if constexpr (Env::cfg(FEAT::MAINCB))
				return SDL_APP_SUCCESS;
			else
				SDL_AppQuit(this, SDL_APP_SUCCESS);

		case SDL_EVENT_WINDOW_FOCUS_GAINED:
			m_bHasFocus = true;
			m_engine->onFocusGained();
			setFgFPS();
			break;

		case SDL_EVENT_WINDOW_FOCUS_LOST:
			m_bHasFocus = false;
			m_engine->onFocusLost();
			setBgFPS();
			break;

		case SDL_EVENT_WINDOW_MAXIMIZED:
			m_bMinimized = false;
			m_engine->onMaximized();
			setFgFPS();
			break;

		case SDL_EVENT_WINDOW_MINIMIZED:
			m_bMinimized = true;
			m_engine->onMinimized();
			setBgFPS();
			break;

		case SDL_EVENT_WINDOW_RESTORED:
			m_bMinimized = false;
			m_engine->onRestored();
			setFgFPS();
			break;

		case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
		case SDL_EVENT_WINDOW_RESIZED:
			m_engine->requestResolutionChange(Vector2(static_cast<float>(event->window.data1), static_cast<float>(event->window.data2)));
			setFgFPS();
			break;

		default:
			break;
		}
		break;

	// keyboard events
	case SDL_EVENT_KEY_DOWN:
		m_engine->onKeyboardKeyDown(event->key.scancode);
		break;

	case SDL_EVENT_KEY_UP:
		m_engine->onKeyboardKeyUp(event->key.scancode);
		break;

	case SDL_EVENT_TEXT_INPUT:
		for (const auto &key : UString(event->text.text))
			m_engine->onKeyboardChar(key);
		break;

	// mouse events
	case SDL_EVENT_MOUSE_BUTTON_DOWN:
		m_engine->onMouseButtonChange(event->button.button, true);
		break;

	case SDL_EVENT_MOUSE_BUTTON_UP:
		m_engine->onMouseButtonChange(event->button.button, false);
		break;

	case SDL_EVENT_MOUSE_WHEEL:
		if (event->wheel.x != 0)
			m_engine->onMouseWheelHorizontal(event->wheel.x > 0 ? 120 * std::abs(static_cast<int>(event->wheel.x)) : -120 * std::abs(static_cast<int>(event->wheel.x)));
		if (event->wheel.y != 0)
			m_engine->onMouseWheelVertical(event->wheel.y > 0 ? 120 * std::abs(static_cast<int>(event->wheel.y)) : -120 * std::abs(static_cast<int>(event->wheel.y)));
		break;

	case SDL_EVENT_MOUSE_MOTION:
		// cache the position
		m_vLastRelMousePos.x = event->motion.xrel;
		m_vLastRelMousePos.y = event->motion.yrel;
		m_vLastAbsMousePos.x = event->motion.x;
		m_vLastAbsMousePos.y = event->motion.y;
		m_engine->onMouseMotion(event->motion.x, event->motion.y, event->motion.xrel, event->motion.yrel, event->motion.which != 0);
		break;

	default:
		break;
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDLMain::iterate()
{
	if (!m_bRunning)
		return SDL_APP_SUCCESS;

	// update
	{
		m_deltaTimer->update();
		m_engine->setFrameTime(m_deltaTimer->getDelta());
		m_engine->onUpdate();
	}

	// draw
	{
		m_bDrawing = true;
		m_engine->onPaint();
		m_bDrawing = false;
	}

	if constexpr (!Env::cfg(FEAT::MAINCB)) // main callbacks use SDL iteration rate to limit fps
	{
		VPROF_BUDGET("FPSLimiter", VPROF_BUDGETGROUP_SLEEP);

		// if minimized or unfocused, use BG fps, otherwise use fps_max (if 0 it's unlimited)
		const int targetFPS = m_bMinimized || !m_bHasFocus ? m_iFpsMaxBG : m_iFpsMax;

		if (targetFPS > 0)
		{
			const uint64_t frameTimeNS = SDL_NS_PER_SECOND / static_cast<uint64_t>(targetFPS);
			const uint64_t now = SDL_GetTicksNS();

			// if we're ahead of schedule, sleep until next frame
			if (m_iNextFrameTime > now)
			{
				const uint64_t sleepTime = m_iNextFrameTime - now;
				SDL_DelayPrecise(sleepTime);
			}
			else
			{
				// behind schedule or exactly on time, reset to now
				m_iNextFrameTime = now;
			}
			// set time for next frame
			m_iNextFrameTime += frameTimeNS;
		}
	}

	return SDL_APP_CONTINUE;
}

bool SDLMain::createWindow(int width, int height)
{
	// pre window-creation settings
	if constexpr (Env::cfg((REND::GL | REND::GLES2 | REND::GLES32 | REND::GL3), !REND::DX11))
	{
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, Env::cfg(REND::GL) ? SDL_GL_CONTEXT_PROFILE_COMPATIBILITY : SDL_GL_CONTEXT_PROFILE_ES);
		if constexpr (!Env::cfg(REND::GL))
		{
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, Env::cfg(REND::GLES2) ? 2 : 3);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
		}
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	}

	constexpr auto windowFlags = SDL_WINDOW_HIDDEN | SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS |
	                             ((Env::cfg((REND::GL | REND::GLES2 | REND::GLES32 | REND::GL3), !REND::DX11)) ? SDL_WINDOW_OPENGL
	                              : (Env::cfg(REND::VK, !REND::DX11))                              ? SDL_WINDOW_VULKAN
	                                                                                               : 0UL);

	SDL_PropertiesID props = SDL_CreateProperties();
	SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, WINDOW_TITLE);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_CENTERED);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_CENTERED);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, width);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, height);
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

	// setup some common app metadata
	SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, PACKAGE_NAME);
	SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_VERSION_STRING, PACKAGE_VERSION);
	SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_IDENTIFIER_STRING, "com.mcengine." PACKAGE_NAME);
	SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_CREATOR_STRING, PACKAGE_BUGREPORT);
	SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_COPYRIGHT_STRING, "MIT");
	SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_URL_STRING, PACKAGE_URL);
	SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_TYPE_STRING, "game");

	// create window
	m_window = SDL_CreateWindowWithProperties(props);
	SDL_DestroyProperties(props);

	if (m_window == NULL)
	{
		debugLog("Couldn't SDL_CreateWindow(): %s\n", SDL_GetError());
		return false;
	}

	return true;
}

void SDLMain::setupOpenGL()
{
	if constexpr (Env::cfg((REND::GL | REND::GLES2 | REND::GLES32 | REND::GL3), !REND::DX11))
	{
		m_context = SDL_GL_CreateContext(m_window);
		SDL_GL_MakeCurrent(m_window, m_context);
	}
}

void SDLMain::configureEvents()
{
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
}

void SDLMain::setupLogging()
{
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
		    printf("SDL[%s]: %s\n", catStr, message);
	    },
	    nullptr);
}

void SDLMain::parseArgs()
{
#if defined(MCENGINE_PLATFORM_WINDOWS) || (defined(_WIN32) && !defined(__linux__))
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
		auto pSetProcessDPIAware = (PSPDA)GetProcAddress(GetModuleHandle(TEXT("user32.dll")), "SetProcessDPIAware");
		if (pSetProcessDPIAware != NULL)
			pSetProcessDPIAware();
	}
#else
	return;
#endif
}

void SDLMain::shutdown(SDL_AppResult result)
{
	if (result == SDL_APP_FAILURE) // force quit now
		return;
	else if (m_window)
		SDL_StopTextInput(m_window);
	shutdown(); // engine will be deleted by parent destructor
}

// convar change callbacks, to set app iteration rate
void SDLMain::fps_max_callback(UString, UString newVal)
{
	int newFps = newVal.toInt();
	if ((newFps == 0 || newFps > 30) && !fps_unlimited.getBool())
		m_iFpsMax = newFps;
	if (m_bHasFocus)
		setFgFPS();
}

void SDLMain::fps_max_background_callback(UString, UString newVal)
{
	int newFps = newVal.toInt();
	if (newFps >= 0)
		m_iFpsMaxBG = newFps;
	if (!m_bHasFocus)
		setBgFPS();
}

void SDLMain::fps_unlimited_callback(UString, UString newVal)
{
	if (newVal.toBool())
		m_iFpsMax = 0;
	else
		m_iFpsMax = fps_max.getInt();
	if (m_bHasFocus)
		setFgFPS();
}
