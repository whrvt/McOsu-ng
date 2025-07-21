//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		cross-platform main() entrypoint
//
// $NoKeywords: $sdlcallbacks $main
//===============================================================================//

#include "BaseEnvironment.h"

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
namespace
{
void setcwdexe(const char * /*unused*/) {}
} // namespace
#else
#define MAIN_FUNC int main(int argc, char *argv[])
#define nocbinline forceinline
#include <filesystem>
namespace
{
namespace fs = std::filesystem;
void setcwdexe(const char *argv0) noexcept
{
	std::error_code ec;
	fs::path exe_path{};
	if constexpr (Env::cfg(OS::LINUX))
		exe_path = fs::canonical("/proc/self/exe", ec);
	else if constexpr (Env::cfg(OS::WINDOWS))
		exe_path = fs::canonical(fs::path(argv0), ec);

	if (ec || !exe_path.has_parent_path())
		return;

	fs::current_path(exe_path.parent_path(), ec);
}
} // namespace
#endif

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "Engine.h"
#include "Environment.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Profiler.h"
#include "Timing.h"
#include "FPSLimiter.h"

// thin environment subclass to provide SDL callbacks with direct access to members
class SDLMain final : public Environment
{
public:
	SDLMain(int argc, char *argv[]);
	~SDLMain();

	SDLMain &operator=(const SDLMain &) = delete;
	SDLMain &operator=(SDLMain &&) = delete;
	SDLMain(const SDLMain &) = delete;
	SDLMain(SDLMain &&) = delete;

	SDL_AppResult initialize();
	SDL_AppResult iterate();
	SDL_AppResult handleEvent(SDL_Event *event);
	void shutdown(SDL_AppResult result);

private:
	// window and context
	SDL_GLContext m_context;

	// engine update timer
	Timer *m_deltaTimer;

	int m_iFpsMax;
	int m_iFpsMaxBG;

	// set iteration rate for callbacks
	// clang-format off
	inline void setFgFPS() { if constexpr (Env::cfg(FEAT::MAINCB)) SDL_SetHint(SDL_HINT_MAIN_CALLBACK_RATE, fmt::format("{}", m_iFpsMax).c_str()); else FPSLimiter::reset(); }
	inline void setBgFPS() { if constexpr (Env::cfg(FEAT::MAINCB)) SDL_SetHint(SDL_HINT_MAIN_CALLBACK_RATE, fmt::format("{}", m_iFpsMaxBG).c_str()); }
	// clang-format on

	// init methods
	void setupLogging();
	bool createWindow();
	void setupOpenGL();
	void configureEvents();
	float queryDisplayHz();

	// callback handlers
	void fps_max_callback(float newVal);
	void fps_max_background_callback(float newVal);
	void fps_unlimited_callback(float newVal);

	void doEarlyCmdlineOverrides();
};

//*********************************//
//	SDL CALLBACKS/MAINLOOP BEGINS  //
//*********************************//

// called when the SDL_APP_SUCCESS (normal exit) or SDL_APP_FAILURE (something bad happened) event is returned from Init/Iterate/Event
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
	if (result == SDL_APP_FAILURE)
	{
		fprintf(stderr, "[main]: Force exiting now, a fatal error occurred. (SDL error: %s)\n", SDL_GetError());
		std::abort();
	}

	auto *fmain = static_cast<SDLMain *>(appstate);
	fmain->shutdown(result);
	SAFE_DELETE(fmain);

	printf("[main]: Shutdown success.\n");

	if constexpr (!Env::cfg(FEAT::MAINCB))
	{
		SDL_Quit();
		std::exit(0);
	}
}

// we can just call handleEvent and iterate directly if we're not using main callbacks
#if defined(MCENGINE_PLATFORM_WASM) || defined(MCENGINE_FEATURE_MAINCALLBACKS)
// (event queue processing) serialized with SDL_AppIterate
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
	return static_cast<SDLMain *>(appstate)->handleEvent(event);
}

// (update tick) serialized with SDL_AppEvent
SDL_AppResult SDL_AppIterate(void *appstate)
{
	return static_cast<SDLMain *>(appstate)->iterate();
}
#endif

// actual main/init, called once
MAIN_FUNC /* int argc, char *argv[] */
{
	if constexpr (!Env::cfg(OS::WASM))
		setcwdexe(argv[0]); // set the current working directory to the executable directory, so that relative paths work as expected

	std::string lowerPackageName = PACKAGE_NAME;
	std::ranges::transform(lowerPackageName, lowerPackageName.begin(), [](char c) { return std::tolower(c); });

	// setup some common app metadata (SDL says these should be called as early as possible)
	SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, PACKAGE_NAME);
	SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_VERSION_STRING, PACKAGE_VERSION);
	SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_IDENTIFIER_STRING, fmt::format("com.mcengine.{}", lowerPackageName).c_str());
	SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_CREATOR_STRING, PACKAGE_BUGREPORT);
	SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_COPYRIGHT_STRING, "MIT/GPL3"); // mcosu is gpl3, mcengine is mit
	SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_URL_STRING, PACKAGE_URL);
	SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_TYPE_STRING, "game");

	SDL_SetHint(SDL_HINT_VIDEO_DOUBLE_BUFFER, "1");
	if (!SDL_Init(SDL_INIT_VIDEO)) // other subsystems can be init later
	{
		fprintf(stderr, "Couldn't SDL_Init(): %s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	auto *fmain = new SDLMain(argc, argv);

#if !(defined(MCENGINE_PLATFORM_WASM) || defined(MCENGINE_FEATURE_MAINCALLBACKS))
	if (!fmain || fmain->initialize() == SDL_APP_FAILURE)
		SDL_AppQuit(fmain, SDL_APP_FAILURE);

	constexpr int SIZE_EVENTS = 64;
	std::array<SDL_Event, SIZE_EVENTS> events{};

	while (fmain->isRunning())
	{
		VPROF_MAIN();
		{
			// event collection
			VPROF_BUDGET("SDL", VPROF_BUDGETGROUP_WNDPROC);
			int eventCount = 0;
			{
				VPROF_BUDGET("SDL_PumpEvents", VPROF_BUDGETGROUP_WNDPROC);
				SDL_PumpEvents();
			}
			do
			{
				{
					VPROF_BUDGET("SDL_PeepEvents", VPROF_BUDGETGROUP_WNDPROC);
					eventCount = SDL_PeepEvents(&events[0], SIZE_EVENTS, SDL_GETEVENT, SDL_EVENT_FIRST, SDL_EVENT_LAST);
				}
				{
					VPROF_BUDGET("handleEvent", VPROF_BUDGETGROUP_WNDPROC);
					for (int i = 0; i < eventCount; ++i)
						fmain->handleEvent(&events[i]);
				}
			} while (eventCount == SIZE_EVENTS);
		}
		{
			// engine update + draw + fps limiter
			fmain->iterate();
		}
	}

	return 0;
#else
	*appstate = fmain;
	return fmain->initialize();
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
namespace cv
{
ConVar fps_max("fps_max", 360.0f, FCVAR_NONE, "framerate limiter, foreground");
ConVar fps_max_background("fps_max_background", 30.0f, FCVAR_NONE, "framerate limiter, background");
ConVar fps_unlimited("fps_unlimited", false, FCVAR_NONE);

ConVar fps_yield("fps_yield", true, FCVAR_NONE, "always release rest of timeslice at the end of each frame (call scheduler via sleep(0))");
} // namespace cv

SDLMain::SDLMain(int argc, char *argv[])
    : Environment(argc, argv)
{
	m_context = nullptr;
	m_deltaTimer = nullptr;

	m_iFpsMax = 360;
	m_iFpsMaxBG = 30;

	// setup callbacks
	cv::fps_max.setCallback(fastdelegate::MakeDelegate(this, &SDLMain::fps_max_callback));
	cv::fps_max_background.setCallback(fastdelegate::MakeDelegate(this, &SDLMain::fps_max_background_callback));
	cv::fps_unlimited.setCallback(fastdelegate::MakeDelegate(this, &SDLMain::fps_unlimited_callback));
}

SDLMain::~SDLMain()
{
	// clean up timers
	SAFE_DELETE(m_deltaTimer);

	// stop the engine
	SAFE_DELETE(m_engine);

	// clean up GL context
	if (m_context && (Env::cfg((REND::GL | REND::GLES32 | REND::GL3), !REND::DX11)))
	{
		SDL_GL_DestroyContext(m_context);
		m_context = nullptr;
	}
	// close/delete the window
	if (m_window)
	{
		SDL_DestroyWindow(m_window);
		m_window = nullptr;
	}
}

SDL_AppResult SDLMain::initialize()
{
	doEarlyCmdlineOverrides();
	setupLogging();

	// create window with props
	if (!createWindow())
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

	// initialize with the display refresh rate of the current monitor
	m_fDisplayHzSecs = 1.0f / (m_fDisplayHz = queryDisplayHz());

	// init timing
	m_deltaTimer = new Timer(false);

	// initialize engine, now that all the setup is done
	m_engine = Environment::initEngine();

	// start engine frame timer
	m_deltaTimer->start();
	m_deltaTimer->update();

	// if we got to this point, all relevant subsystems (input handling, graphics interface, etc.) have been initialized

	// make window visible
	SDL_ShowWindow(m_window);
	SDL_RaiseWindow(m_window);

	// load app
	m_engine->loadApp();

	// SDL3 stops listening to text input globally when window is created
	SDL_StartTextInput(m_window);
	SDL_SetWindowKeyboardGrab(m_window, false); // this allows windows key and such to work

	// return init success
	return SDL_APP_CONTINUE;
}

static_assert(SDL_EVENT_WINDOW_FIRST == SDL_EVENT_WINDOW_SHOWN);
static_assert(SDL_EVENT_WINDOW_LAST == SDL_EVENT_WINDOW_HDR_STATE_CHANGED);

nocbinline SDL_AppResult SDLMain::handleEvent(SDL_Event *event)
{
	switch (event->type)
	{
	case SDL_EVENT_QUIT:
		if (m_bRunning)
		{
			m_bRunning = false;
			m_engine->shutdown();
			if constexpr (Env::cfg(FEAT::MAINCB))
				return SDL_APP_SUCCESS;
			else
				SDL_AppQuit(this, SDL_APP_SUCCESS);
		}
		break;

	// window events (i hate you msvc ffs)
	// clang-format off
	case SDL_EVENT_WINDOW_SHOWN:				 case SDL_EVENT_WINDOW_HIDDEN:			  case SDL_EVENT_WINDOW_EXPOSED:
	case SDL_EVENT_WINDOW_MOVED:				 case SDL_EVENT_WINDOW_RESIZED:			  case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
	case SDL_EVENT_WINDOW_METAL_VIEW_RESIZED:	 case SDL_EVENT_WINDOW_MINIMIZED:		  case SDL_EVENT_WINDOW_MAXIMIZED:
	case SDL_EVENT_WINDOW_RESTORED:				 case SDL_EVENT_WINDOW_MOUSE_ENTER:		  case SDL_EVENT_WINDOW_MOUSE_LEAVE:
	case SDL_EVENT_WINDOW_FOCUS_GAINED:			 case SDL_EVENT_WINDOW_FOCUS_LOST:		  case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
	case SDL_EVENT_WINDOW_HIT_TEST:				 case SDL_EVENT_WINDOW_ICCPROF_CHANGED:	  case SDL_EVENT_WINDOW_DISPLAY_CHANGED:
	case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED: case SDL_EVENT_WINDOW_SAFE_AREA_CHANGED: case SDL_EVENT_WINDOW_OCCLUDED:
	case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:		 case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:  case SDL_EVENT_WINDOW_DESTROYED:
	case SDL_EVENT_WINDOW_HDR_STATE_CHANGED:
		// clang-format on
		switch (event->window.type)
		{
		case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
			if (m_bRunning)
			{
				m_bRunning = false;
				m_engine->shutdown();
				if constexpr (Env::cfg(FEAT::MAINCB))
					return SDL_APP_SUCCESS;
				else
					SDL_AppQuit(this, SDL_APP_SUCCESS);
			}
			break;

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
			m_bHasFocus = true;
			m_engine->onMaximized();
			setFgFPS();
			break;

		case SDL_EVENT_WINDOW_MINIMIZED:
			m_bMinimized = true;
			m_bHasFocus = false;
			m_engine->onMinimized();
			setBgFPS();
			break;

		case SDL_EVENT_WINDOW_RESTORED:
			m_bMinimized = false;
			m_bHasFocus = true;
			m_engine->onRestored();
			setFgFPS();
			break;

		case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
			cv::fullscreen.setValue(true, false);
			m_bFullscreen = true;
			break;

		case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:
			cv::fullscreen.setValue(false, false);
			m_bFullscreen = false;
			break;

		case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
		case SDL_EVENT_WINDOW_RESIZED:
			m_bHasFocus = true;
			m_fDisplayHzSecs = 1.0f / (m_fDisplayHz = queryDisplayHz());
			m_engine->requestResolutionChange(Vector2(static_cast<float>(event->window.data1), static_cast<float>(event->window.data2)));
			setFgFPS();
			break;

		case SDL_EVENT_WINDOW_DISPLAY_CHANGED:
			cv::monitor.setValue(event->window.data1, false);
		case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED: // TODO?
			m_engine->requestResolutionChange(getWindowSize());
			m_fDisplayHzSecs = 1.0f / (m_fDisplayHz = queryDisplayHz());
			break;

		default:
			if (m_bEnvDebug)
				debugLog("DEBUG: unhandled SDL window event {}\n", static_cast<int>(event->window.type));
			break;
		}
		break;

	// keyboard events
	case SDL_EVENT_KEY_DOWN:
		keyboard->onKeyDown(event->key.scancode);
		break;

	case SDL_EVENT_KEY_UP:
		keyboard->onKeyUp(event->key.scancode);
		break;

	case SDL_EVENT_TEXT_INPUT:
		for (const auto &key : UString(event->text.text))
			keyboard->onChar(key);
		break;

	// mouse events
	case SDL_EVENT_MOUSE_BUTTON_DOWN:
		mouse->onButtonChange(static_cast<MouseButton::Index>(event->button.button), true); // C++ needs me to cast an unsigned char to an unsigned char
		break;

	case SDL_EVENT_MOUSE_BUTTON_UP:
		mouse->onButtonChange(static_cast<MouseButton::Index>(event->button.button), false);
		break;

	case SDL_EVENT_MOUSE_WHEEL:
		if (event->wheel.x != 0)
			mouse->onWheelHorizontal(event->wheel.x > 0 ? 120 * std::abs(static_cast<int>(event->wheel.x)) : -120 * std::abs(static_cast<int>(event->wheel.x)));
		if (event->wheel.y != 0)
			mouse->onWheelVertical(event->wheel.y > 0 ? 120 * std::abs(static_cast<int>(event->wheel.y)) : -120 * std::abs(static_cast<int>(event->wheel.y)));
		break;

	case SDL_EVENT_MOUSE_MOTION:
		// debugLog("mouse motion on frame {}\n", m_engine->getFrameCount());
		//  cache the position
		m_vLastRelMousePos.x = event->motion.xrel;
		m_vLastRelMousePos.y = event->motion.yrel;
		m_vLastAbsMousePos.x = event->motion.x;
		m_vLastAbsMousePos.y = event->motion.y;
		mouse->onMotion(event->motion.x, event->motion.y, event->motion.xrel, event->motion.yrel, event->motion.which != 0);
		break;

	default:
		if (m_bEnvDebug)
			debugLog("DEBUG: unhandled SDL event {}\n", static_cast<int>(event->type));
		break;
	}

	return SDL_APP_CONTINUE;
}

nocbinline SDL_AppResult SDLMain::iterate()
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
		FPSLimiter::limitFrames(targetFPS);
	}

	return SDL_APP_CONTINUE;
}

bool SDLMain::createWindow()
{
	// pre window-creation settings
	if constexpr (Env::cfg((REND::GL | REND::GLES32 | REND::GL3), !REND::DX11))
	{
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
		                    Env::cfg(REND::GL) ? SDL_GL_CONTEXT_PROFILE_COMPATIBILITY
		                                       : (Env::cfg(REND::GL3) ? SDL_GL_CONTEXT_PROFILE_CORE : SDL_GL_CONTEXT_PROFILE_ES));
		if constexpr (!Env::cfg(REND::GL | REND::GL3))
		{
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
		}
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	}

	// set vulkan for linux dxvk-native, opengl otherwise (or none for windows dx11)
	constexpr auto windowFlags = SDL_WINDOW_HIDDEN | SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS |
	                             (Env::cfg((REND::GL | REND::GLES32 | REND::GL3)) ? SDL_WINDOW_OPENGL : (Env::cfg(OS::LINUX, REND::DX11) ? SDL_WINDOW_VULKAN : 0UL));

	// get default monitor resolution and create the window with that as the starting size
	long windowCreateWidth = WINDOW_WIDTH;
	long windowCreateHeight = WINDOW_HEIGHT;
	{
		SDL_DisplayID di = SDL_GetPrimaryDisplay();
		const SDL_DisplayMode *dm = nullptr;
		if (di && (dm = SDL_GetDesktopDisplayMode(di)))
		{
			windowCreateWidth = dm->w;
			windowCreateHeight = dm->h;
		}
	}

	SDL_PropertiesID props = SDL_CreateProperties();
	// if constexpr (Env::cfg(REND::DX11))
	// 	SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_EXTERNAL_GRAPHICS_CONTEXT_BOOLEAN, true);
	SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, WINDOW_TITLE);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_CENTERED);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_CENTERED);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, windowCreateWidth);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, windowCreateHeight);
	SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, false);
	SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_MAXIMIZED_BOOLEAN, false);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_FLAGS_NUMBER, windowFlags);

	const bool shouldBeBorderless = cv::fullscreen_windowed_borderless.getBool();
	// FIXME: no configs are loaded here yet, so this is pointless (and we don't want to create fullscreen because we don't want a video mode change)
	const bool shouldBeFullscreen = !Env::cfg(OS::WINDOWS, REND::DX11) && (cv::fullscreen.getBool() || !cv::windowed.getBool() || shouldBeBorderless);

	SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_BORDERLESS_BOOLEAN, shouldBeBorderless);
	SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_FULLSCREEN_BOOLEAN, shouldBeFullscreen);

	if constexpr (Env::cfg(OS::WINDOWS))
		SDL_SetHintWithPriority(SDL_HINT_WINDOWS_RAW_KEYBOARD, "1", SDL_HINT_OVERRIDE);
	else
		SDL_SetHintWithPriority(SDL_HINT_MOUSE_AUTO_CAPTURE, "0", SDL_HINT_OVERRIDE);
	SDL_SetHintWithPriority(SDL_HINT_MOUSE_RELATIVE_MODE_CENTER, "0", SDL_HINT_OVERRIDE);
	SDL_SetHintWithPriority(SDL_HINT_TOUCH_MOUSE_EVENTS, "0", SDL_HINT_OVERRIDE);
	SDL_SetHintWithPriority(SDL_HINT_MOUSE_EMULATE_WARP_WITH_RELATIVE, "0", SDL_HINT_OVERRIDE);

	// create window
	m_window = SDL_CreateWindowWithProperties(props);
	SDL_DestroyProperties(props);

	if (m_window == NULL)
	{
		debugLog("Couldn't SDL_CreateWindow(): {:s}\n", SDL_GetError());
		return false;
	}

	cv::monitor.setValue(SDL_GetDisplayForWindow(m_window), false);

	return true;
}

void SDLMain::setupOpenGL()
{
	if constexpr (Env::cfg((REND::GL | REND::GLES32 | REND::GL3), !REND::DX11))
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

float SDLMain::queryDisplayHz()
{
	// get the screen refresh rate, and set fps_max to that as default
	if constexpr (!Env::cfg(OS::WASM)) // not in WASM
	{
		const SDL_DisplayID display = SDL_GetDisplayForWindow(m_window);
		const SDL_DisplayMode *currentDisplayMode = display ? SDL_GetCurrentDisplayMode(display) : nullptr;

		if (currentDisplayMode && currentDisplayMode->refresh_rate > 0)
		{
			if (!almostEqual(m_fDisplayHz, currentDisplayMode->refresh_rate))
				debugLog("Got refresh rate {:.3f} Hz for display {:d}.\n", currentDisplayMode->refresh_rate, display);
			const auto refreshRateSanityClamped = std::clamp<float>(currentDisplayMode->refresh_rate, 60.0f, 540.0f);
			const auto fourxhz = std::clamp<float>(refreshRateSanityClamped * 4.0f, refreshRateSanityClamped, 1000.0f);
			// also set fps_max to 4x the refresh rate if it's the default
			if (cv::fps_max.getFloat() == cv::fps_max.getDefaultFloat())
				cv::fps_max.setValue(fourxhz);
			return refreshRateSanityClamped;
		}
		else
		{
			static int once;
			if (!once++)
				debugLog("Couldn't SDL_GetCurrentDisplayMode(SDL display: {:d}): {:s}\n", display, SDL_GetError());
		}
	}
	// in wasm or if we couldn't get the refresh rate just return a sane value to use for "vsync"-related calculations
	return std::clamp<float>(cv::fps_max.getFloat(), 60.0f, 360.0f);
}

void SDLMain::setupLogging()
{
	SDL_SetLogOutputFunction(
	    [](void *, int category, SDL_LogPriority, const char *message) {
		    const char *catStr = "???";
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
			    break;
		    }
		    printf("SDL[%s]: %s\n", catStr, message);
	    },
	    nullptr);
}

void SDLMain::doEarlyCmdlineOverrides()
{
#if defined(MCENGINE_PLATFORM_WINDOWS) || (defined(_WIN32) && !defined(__linux__))
	// disable IME text input if -noime (or if the feature won't be supported)
#ifdef MCENGINE_FEATURE_IMESUPPORT
	if (m_mArgMap.contains("-noime"))
#endif
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
	if (!m_mArgMap.contains("-nodpi"))
	{
		typedef BOOL(WINAPI * PSPDA)(void);
		auto pSetProcessDPIAware = (PSPDA)GetProcAddress(GetModuleHandle(TEXT("user32.dll")), "SetProcessDPIAware");
		if (pSetProcessDPIAware != NULL)
			pSetProcessDPIAware();
	}
#else
	// nothing yet
	return;
#endif
}

void SDLMain::shutdown(SDL_AppResult result)
{
	if (result == SDL_APP_FAILURE) // force quit now
		return;
	else if (m_window)
		SDL_StopTextInput(m_window);

	Environment::shutdown();
}

// convar change callbacks, to set app iteration rate
void SDLMain::fps_max_callback(float newVal)
{
	int newFps = static_cast<int>(newVal);
	if ((newFps == 0 || newFps >= 30) && !cv::fps_unlimited.getBool())
		m_iFpsMax = newFps;
	if (m_bHasFocus)
		setFgFPS();
}

void SDLMain::fps_max_background_callback(float newVal)
{
	int newFps = static_cast<int>(newVal);
	if (newFps >= 0)
		m_iFpsMaxBG = newFps;
	if (!m_bHasFocus)
		setBgFPS();
}

void SDLMain::fps_unlimited_callback(float newVal)
{
	if (newVal > 0.0f)
		m_iFpsMax = 0;
	else
		m_iFpsMax = cv::fps_max.getInt();
	if (m_bHasFocus)
		setFgFPS();
}
