//================ Copyright (c) 2018, PG, All rights reserved. =================//
//
// Purpose:		main entry point
//
// $NoKeywords: $main
//===============================================================================//

#include "EngineFeatures.h"

#ifdef MCENGINE_FEATURE_SDL

//#define MCENGINE_SDL_JOYSTICK
//#define MCENGINE_SDL_JOYSTICK_MOUSE

#if !defined(MCENGINE_FEATURE_OPENGL) && !defined(MCENGINE_FEATURE_OPENGLES)
#error OpenGL support is currently required for SDL
#endif

#include <SDL3/SDL.h>

#include "Engine.h"
#include "Profiler.h"
#include "Timer.h"
#include "Mouse.h"
#include "ConVar.h"
#include "ConsoleBox.h"

#include "SDLEnvironment.h"
#include "HorizonSDLEnvironment.h"
#include "WinSDLEnvironment.h"



#define WINDOW_TITLE "McEngine"

#define WINDOW_WIDTH (1280)
#define WINDOW_HEIGHT (720)

#define WINDOW_WIDTH_MIN 100
#define WINDOW_HEIGHT_MIN 100

static constexpr auto SIZE_EVENTS = 64;

Engine *g_engine = NULL;

bool g_bRunning = true;
bool g_bUpdate = true;
bool g_bDraw = true;
bool g_bDrawing = false;

bool g_bMinimized = false; // for fps_max_background
bool g_bHasFocus = true; // for fps_max_background

SDL_Window *g_window = NULL;

ConVar fps_max("fps_max", 60.0f, FCVAR_NONE, "framerate limiter, foreground");
ConVar fps_max_yield("fps_max_yield", true, FCVAR_NONE, "always release rest of timeslice once per frame (call scheduler via sleep(0))");
ConVar fps_max_background("fps_max_background", 30.0f, FCVAR_NONE, "framerate limiter, background");
ConVar fps_unlimited("fps_unlimited", false, FCVAR_NONE);
ConVar fps_unlimited_yield("fps_unlimited_yield", true, FCVAR_NONE, "always release rest of timeslice once per frame (call scheduler via sleep(0)), even if unlimited fps are enabled");

ConVar sdl_joystick_mouse_sensitivity("sdl_joystick_mouse_sensitivity", 1.0f, FCVAR_NONE);
ConVar sdl_joystick0_deadzone("sdl_joystick0_deadzone", 0.3f, FCVAR_NONE);
ConVar sdl_joystick_zl_threshold("sdl_joystick_zl_threshold", -0.5f, FCVAR_NONE);
ConVar sdl_joystick_zr_threshold("sdl_joystick_zr_threshold", -0.5f, FCVAR_NONE);

#ifdef MCENGINE_SDL_TOUCHSUPPORT
//ConVar sdl_steamdeck_starttextinput_workaround("sdl_steamdeck_starttextinput_workaround", true, FCVAR_NONE, "currently used to fix an SDL2 bug on the Steam Deck (fixes USB/touch keyboard textual input not working under gamescope), see https://github.com/libsdl-org/SDL/issues/8561");
ConVar sdl_steamdeck_doubletouch_workaround("sdl_steamdeck_doubletouch_workaround", true, FCVAR_NONE, "currently used to fix a Valve/SDL2 bug on the Steam Deck (fixes \"Touchscreen Native Support\" firing 4 events for one single touch under gamescope, i.e. DOWN/UP/DOWN/UP instead of just DOWN/UP)");

static bool isSteamDeckInt()
{
	const char *steamDeck = std::getenv("SteamDeck");
	if (steamDeck != NULL)
	{
		const std::string stdSteamDeck(steamDeck);
		return (stdSteamDeck == "1");
	}
	return false;
}

static bool isGamescopeInt()
{
	const char *xdgCurrentDesktop = std::getenv("XDG_CURRENT_DESKTOP");
	if (xdgCurrentDesktop != NULL)
	{
		const std::string stdXdgCurrentDesktop(xdgCurrentDesktop);
		return (stdXdgCurrentDesktop == "gamescope");
	}
	return false;
}
#define deckTouchHack (isSteamDeck && environment->wasLastMouseInputTouch())
#else
#define deckTouchHack false
#endif

static const char *getCategoryString(int category)
{
	switch (category)
	{
	case SDL_LOG_CATEGORY_APPLICATION:
		return "APP";
	case SDL_LOG_CATEGORY_ERROR:
		return "ERR";
	case SDL_LOG_CATEGORY_SYSTEM:
		return "SYS";
	case SDL_LOG_CATEGORY_AUDIO:
		return "AUD";
	case SDL_LOG_CATEGORY_VIDEO:
		return "VID";
	case SDL_LOG_CATEGORY_RENDER:
		return "REN";
	case SDL_LOG_CATEGORY_INPUT:
		return "INP";
	case SDL_LOG_CATEGORY_CUSTOM:
		return "USR";
	default:
		return "???";
	}
}

static void SDLLogCallback(void *, int category, SDL_LogPriority, const char *message)
{
	const char *catStr = getCategoryString(category);
	fprintf(stderr, "SDL[%s]: %s\n", catStr, message);
}

int mainSDL(int argc, char *argv[], SDLEnvironment *customSDLEnvironment)
{
	SDLEnvironment *environment = customSDLEnvironment;

	uint32_t flags = SDL_INIT_VIDEO;

#ifdef MCENGINE_SDL_JOYSTICK

	flags |= SDL_INIT_JOYSTICK;

#endif

	// initialize sdl
	if (!SDL_Init(flags))
	{
		debugLog("Couldn't SDL_Init(): %s\n", SDL_GetError());
		return 1;
	}

	SDL_SetLogOutputFunction(SDLLogCallback, nullptr);

	// pre window-creation settings
#ifdef MCENGINE_FEATURE_OPENGL

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

#endif

#ifdef MCENGINE_FEATURE_OPENGLES

	// NOTE: hardcoded to OpenGL ES 2.0 currently (for nintendo switch builds)
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

#endif

	uint32_t windowFlags = SDL_WINDOW_HIDDEN | SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS;

#if defined(MCENGINE_FEATURE_OPENGL) || defined(MCENGINE_FEATURE_OPENGLES)

	windowFlags |= SDL_WINDOW_OPENGL;

#endif

    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, WINDOW_TITLE);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_CENTERED);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_CENTERED);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, WINDOW_WIDTH);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, WINDOW_HEIGHT);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, false);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_BORDERLESS_BOOLEAN, false); // set these props later, after engine starts and loads cfgs
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_FULLSCREEN_BOOLEAN, false);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_MAXIMIZED_BOOLEAN, false);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_FLAGS_NUMBER, windowFlags);

	// create window
    g_window = SDL_CreateWindowWithProperties(props);
    SDL_DestroyProperties(props);

	if (g_window == NULL)
	{
		debugLog("Couldn't SDL_CreateWindow(): %s\n", SDL_GetError());
		return 1;
	}

	// get the screen refresh rate, and set fps_max to that as default
	{
		const SDL_DisplayMode * currentDisplayMode;
		const SDL_DisplayID display = SDL_GetDisplayForWindow(g_window);
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
			fps_max.setValue(fps_max.getInt());
		}
	}

	// create OpenGL context
#if defined(MCENGINE_FEATURE_OPENGL) || defined(MCENGINE_FEATURE_OPENGLES)

	SDL_GLContext context = SDL_GL_CreateContext(g_window);

#endif

#ifdef MCENGINE_SDL_JOYSTICK

	SDL_OpenJoystick(0);
	SDL_OpenJoystick(1);

	float m_fJoystick0XPercent = 0.0f;
	float m_fJoystick0YPercent = 0.0f;

	bool xDown = false;

	bool zlDown = false;
	bool zrDown = false;

	bool hatUpDown = false;
	bool hatDownDown = false;
	bool hatLeftDown = false;
	bool hatRightDown = false;

#endif

    // create timers
    auto *frameTimer = new Timer();
    frameTimer->start();
    frameTimer->update();

    auto *deltaTimer = new Timer();
    deltaTimer->start();
    deltaTimer->update();

	auto *fpsCalcTimer = new Timer();
	fpsCalcTimer->start();
	fpsCalcTimer->update();

	// variables to keep track of fps overhead adjustment
	uint64_t frameCountSinceLastFpsCalc = 0;
	double fpsAdjustment = 0.0;

	// initialize engine
	if (environment == NULL)
		environment = new SDLEnvironment(g_window);
	else
		environment->setWindow(g_window);

	g_engine = new Engine(environment, argc > 1 ? argv[1] : ""); // TODO: proper arg support

	// post window-creation settings
	SDL_SetWindowMinimumSize(g_window, WINDOW_WIDTH_MIN, WINDOW_HEIGHT_MIN);

	const bool shouldBeBorderless = convar->getConVarByName("fullscreen_windowed_borderless")->getBool();
	const bool shouldBeFullscreen = !(convar->getConVarByName("windowed") != NULL) || shouldBeBorderless;

	if (!(shouldBeBorderless || shouldBeFullscreen))
	{
		environment->disableFullscreen();
		environment->center();
	}
	else
	{
		environment->enableFullscreen();
	}

	// make the window visible
	SDL_ShowWindow(g_window);
	SDL_RaiseWindow(g_window);

	g_engine->loadApp();

	frameTimer->update();
	deltaTimer->update();
	fpsCalcTimer->update();

#if defined(MCENGINE_FEATURE_OPENGL) || defined(MCENGINE_FEATURE_OPENGLES)
	SDL_GL_MakeCurrent(g_window, context);
#endif

	// custom
#ifdef MCENGINE_SDL_TOUCHSUPPORT
	const bool isSteamDeckDoubletouchWorkaroundEnabled = (isSteamDeck && isGamescope && sdl_steamdeck_doubletouch_workaround.getBool());
	const bool isSteamDeck = isSteamDeckInt();
	const bool isGamescope = isGamescopeInt();
	std::vector<SDL_FingerID> touchingFingerIds;
	SDL_TouchID currentTouchId = 0;
#endif

	// FIXME: sdl2-sdl3 stops listening to text input globally when window is created,
	// recommended to start/stop listening to text input when required
	SDL_StartTextInput(g_window);

	Vector2 mousePos;
	ConVar *mouse_raw_input_ref = convar->getConVarByName("mouse_raw_input");

	// main loop
	SDL_Event events[SIZE_EVENTS];
	while (g_bRunning)
	{
		VPROF_MAIN();

		// HACKHACK: switch hack (usb mouse/keyboard support)
#ifdef __SWITCH__

		HorizonSDLEnvironment *horizonSDLenv = dynamic_cast<HorizonSDLEnvironment*>(environment);
		if (horizonSDLenv != NULL)
			horizonSDLenv->update_before_winproc();

#endif

		// handle window message queue
		{
			VPROF_BUDGET("SDL", VPROF_BUDGETGROUP_WNDPROC);

			// sanity, ensure any lost SDL_FINGERUP events (even though this should be impossible) don't kill cursor movement for the rest of the session
			// {
			// 	int fingerCount;
			// 	if (SDL_GetTouchFingers(currentTouchId, &fingerCount) && fingerCount < 1) // FIXME: this is wrong, probably (SDL2->SDL3)
			// 		touchingFingerIds.clear();
			// }

			// handle automatic raw input toggling
			{
				const bool isRawInputActuallyEnabled = (SDL_GetWindowRelativeMouseMode(g_window) == true);

				const bool shouldRawInputBeEnabled = (mouse_raw_input_ref->getBool() && !environment->isCursorVisible());

				if (shouldRawInputBeEnabled != isRawInputActuallyEnabled)
                    SDL_SetWindowRelativeMouseMode(g_window, shouldRawInputBeEnabled ? true : false);
			}

			const bool isRawInputEnabled = (SDL_GetWindowRelativeMouseMode(g_window) == true);
			const bool isDebugSdl = false; //environment->sdlDebug();

			SDL_PumpEvents();
			int eventCount;
			do
			{
				eventCount = SDL_PeepEvents(&events[0], SIZE_EVENTS, SDL_GETEVENT, SDL_EVENT_FIRST, SDL_EVENT_LAST);
				for (int i = 0; i < eventCount; ++i)
				{
				switch (events[i].type)
				{
				case SDL_EVENT_QUIT :
					if (g_bRunning)
					{
						g_bRunning = false;
						g_engine->onShutdown();
					}
					break;

				// window
				case SDL_EVENT_WINDOW_FIRST ... SDL_EVENT_WINDOW_LAST:
				{
					switch (events[i].window.type)
					{
					case SDL_EVENT_WINDOW_CLOSE_REQUESTED :
						if (g_bRunning)
						{
							g_bRunning = false;
							g_engine->onShutdown();
						}
						break;
					case SDL_EVENT_WINDOW_FOCUS_GAINED :
						g_bHasFocus = true;
						g_engine->onFocusGained();
						break;
					case SDL_EVENT_WINDOW_FOCUS_LOST :
						g_bHasFocus = false;
						g_engine->onFocusLost();
						break;
					case SDL_EVENT_WINDOW_MAXIMIZED :
						g_bMinimized = false;
						g_engine->onMaximized();
						break;
					case SDL_EVENT_WINDOW_MINIMIZED :
						g_bMinimized = true;
						g_engine->onMinimized();
						break;
					case SDL_EVENT_WINDOW_RESTORED :
						g_bMinimized = false;
						g_engine->onRestored();
						break;
					case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED : case SDL_EVENT_WINDOW_RESIZED :
						g_engine->requestResolutionChange(Vector2(events[i].window.data1, events[i].window.data2));
						break;
					default :
						//debugLog("DEBUG: unhandled window event: %#08x\n", events[i].window.type);
						break;
					}
					break;
				}
				// keyboard
				case SDL_EVENT_KEY_DOWN :
					//debugLog("DEBUG: keydown event: %#08x\n", events[i].key.scancode);
					g_engine->onKeyboardKeyDown(events[i].key.scancode);
					break;

				case SDL_EVENT_KEY_UP :
					//debugLog("DEBUG: keyup event: %#08x\n", events[i].key.scancode);
					g_engine->onKeyboardKeyUp(events[i].key.scancode);
					break;

				case SDL_EVENT_TEXT_INPUT :
					//debugLog("DEBUG: text input event: %s\n", events[i].text.text);
					{
						UString nullTerminatedTextInputString(events[i].text.text);
						for (int i=0; i<nullTerminatedTextInputString.length(); i++)
						{
							g_engine->onKeyboardChar((KEYCODE)nullTerminatedTextInputString[i]); // NOTE: this splits into UTF-16 wchar_t atm
						}
					}
					break;

				// mouse
				case SDL_EVENT_MOUSE_BUTTON_DOWN :
					if (!deckTouchHack) // HACKHACK: Steam Deck workaround (sends mouse events even though native touchscreen support is enabled)
					{
						switch (events[i].button.button)
						{
						case SDL_BUTTON_LEFT:
							g_engine->onMouseLeftChange(true);
							break;
						case SDL_BUTTON_MIDDLE:
							g_engine->onMouseMiddleChange(true);
							break;
						case SDL_BUTTON_RIGHT:
							g_engine->onMouseRightChange(true);
							break;

						case SDL_BUTTON_X1:
							g_engine->onMouseButton4Change(true);
							break;
						case SDL_BUTTON_X2:
							g_engine->onMouseButton5Change(true);
							break;
						}
					}
					break;

				case SDL_EVENT_MOUSE_BUTTON_UP :
					if (!deckTouchHack) // HACKHACK: Steam Deck workaround (sends mouse events even though native touchscreen support is enabled)
					{
						switch (events[i].button.button)
						{
						case SDL_BUTTON_LEFT:
							g_engine->onMouseLeftChange(false);
							break;
						case SDL_BUTTON_MIDDLE:
							g_engine->onMouseMiddleChange(false);
							break;
						case SDL_BUTTON_RIGHT:
							g_engine->onMouseRightChange(false);
							break;

						case SDL_BUTTON_X1:
							g_engine->onMouseButton4Change(false);
							break;
						case SDL_BUTTON_X2:
							g_engine->onMouseButton5Change(false);
							break;
						}
					}
					break;

				case SDL_EVENT_MOUSE_WHEEL :
					if (events[i].wheel.x != 0)
						g_engine->onMouseWheelHorizontal(events[i].wheel.x > 0 ? 120*std::abs(events[i].wheel.x) : -120*std::abs(events[i].wheel.x)); // NOTE: convert to Windows units
					if (events[i].wheel.y != 0)
						g_engine->onMouseWheelVertical(events[i].wheel.y > 0 ? 120*std::abs(events[i].wheel.y) : -120*std::abs(events[i].wheel.y)); // NOTE: convert to Windows units
					break;

				case SDL_EVENT_MOUSE_MOTION :
					if (!deckTouchHack) // HACKHACK: Steam Deck workaround (sends mouse events even though native touchscreen support is enabled)
					{
						if (isDebugSdl)
							debugLog("SDL_MOUSEMOTION: xrel = %f, yrel = %f, which = %i\n", events[i].motion.xrel, events[i].motion.yrel, (int)events[i].motion.which);

						if (events[i].motion.which != SDL_TOUCH_MOUSEID)
						{
							environment->setWasLastMouseInputTouch(false);

							if (isRawInputEnabled)
								g_engine->onMouseRawMove(events[i].motion.xrel, events[i].motion.yrel);
						}
					}
					break;

				// touch mouse
				// NOTE: sometimes when quickly tapping with two fingers, events will get lost (due to the touchscreen believing that it was one finger which moved very quickly, instead of 2 tapping fingers)
#ifdef MCENGINE_SDL_TOUCHSUPPORT
				case SDL_EVENT_FINGER_DOWN :
					{
						if (isDebugSdl)
							debugLog("SDL_FINGERDOWN: touchId = %i, fingerId = %i, x = %f, y = %f\n",
								 (int) events[i].tfinger.touchID,
								 (int) events[i].tfinger.fingerID,
								 events[i].tfinger.x,
								 events[i].tfinger.y);

						environment->setWasLastMouseInputTouch(true);

						currentTouchId = events[i].tfinger.touchID;

						bool isFingerIdAlreadyTouching = false;
						for (const SDL_FingerID &touchingFingerId : touchingFingerIds)
						{
							if (touchingFingerId == events[i].tfinger.fingerID)
							{
								isFingerIdAlreadyTouching = true;
								break;
							}
						}

						if (!isFingerIdAlreadyTouching || isSteamDeckDoubletouchWorkaroundEnabled)
						{
							touchingFingerIds.push_back(events[i].tfinger.fingerID);

							if (!isSteamDeckDoubletouchWorkaroundEnabled || isFingerIdAlreadyTouching)
							{
								if (touchingFingerIds.size() < (isSteamDeckDoubletouchWorkaroundEnabled ? 3 : 2))
								{
									mousePos = Vector2(events[i].tfinger.x, events[i].tfinger.y) * g_engine->getScreenSize();
									environment->setMousePos(mousePos.x, mousePos.y);
									g_engine->getMouse()->onPosChange(mousePos);

									if (g_engine->getMouse()->isLeftDown())
										g_engine->onMouseLeftChange(false);

									g_engine->onMouseLeftChange(true);
								}
								else
								{
									if (g_engine->getMouse()->isLeftDown())
										g_engine->onMouseLeftChange(false);

									g_engine->onMouseLeftChange(true);
								}
							}
						}
					}
					break;

				case SDL_EVENT_FINGER_UP :
					{
						if (isDebugSdl)
							debugLog("SDL_FINGERUP: touchId = %i, fingerId = %i, x = %f, y = %f\n",
								 (int) events[i].tfinger.touchID,
								 (int) events[i].tfinger.fingerID,
								 events[i].tfinger.x,
								 events[i].tfinger.y);

						environment->setWasLastMouseInputTouch(true);

						currentTouchId = events[i].tfinger.touchID;

						// NOTE: also removes the finger from the touchingFingerIds list
						bool wasFingerIdAlreadyTouching = false;
						{
							size_t numFingerIdTouches = 0;
							for (size_t i=0; i<touchingFingerIds.size(); i++)
							{
								if (touchingFingerIds[i] == events[i].tfinger.fingerID)
								{
									wasFingerIdAlreadyTouching = true;
									numFingerIdTouches++;

									if (isSteamDeckDoubletouchWorkaroundEnabled)
										continue;

									touchingFingerIds.erase(touchingFingerIds.begin() + i);
									i--;
								}
							}

							if (isSteamDeckDoubletouchWorkaroundEnabled)
							{
								// cleanup on "last" release (the second one)
								if (numFingerIdTouches > 1)
								{
									for (size_t i=0; i<touchingFingerIds.size(); i++)
									{
										if (touchingFingerIds[i] == events[i].tfinger.fingerID)
										{
											touchingFingerIds.erase(touchingFingerIds.begin() + i);
											i--;
										}
									}
								}
							}
						}

						if (wasFingerIdAlreadyTouching)
						{
							if (events[i].tfinger.fingerID == touchingFingerIds[0])
								g_engine->onMouseLeftChange(false);
						}
					}
					break;

				case SDL_EVENT_FINGER_MOTION :
					{
						if (isDebugSdl)
							debugLog("SDL_FINGERMOTION: touchId = %i, fingerId = %i, x = %f, y = %f, dx = %f, dy = %f\n",
								 (int) events[i].tfinger.touchID,
								 (int) events[i].tfinger.fingerID,
								 events[i].tfinger.x,
								 events[i].tfinger.y,
								 events[i].tfinger.dx,
								 events[i].tfinger.dy);

						environment->setWasLastMouseInputTouch(true);

						currentTouchId = events[i].tfinger.touchID;

						bool isFingerIdTouching = false;
						for (size_t i=0; i<touchingFingerIds.size(); i++)
						{
							if (touchingFingerIds[i] == events[i].tfinger.fingerID)
							{
								isFingerIdTouching = true;
								break;
							}
						}

						if (isFingerIdTouching)
						{
							if (events[i].tfinger.fingerID == touchingFingerIds[0])
							{
								mousePos = Vector2(events[i].tfinger.x, events[i].tfinger.y) * g_engine->getScreenSize();
								environment->setMousePos(mousePos.x, mousePos.y);
								g_engine->getMouse()->onPosChange(mousePos);
							}
						}
					}
					break;
#endif
				// joystick keyboard
				// NOTE: defaults to xbox 360 controller layout on all non-horizon environments
#ifdef MCENGINE_SDL_JOYSTICK

				case SDL_EVENT_JOYSTICK_BUTTON_DOWN :
					if (isDebugSdl)
						debugLog("SDL_JOYBUTTONDOWN: joystickId = %i, button = %i\n", (int)events[i].jbutton.which, (int)events[i].jbutton.button);

					if (events[i].jbutton.button == 0) // KEY_A
					{
						g_engine->onMouseLeftChange(true);

						if (engine->getConsoleBox()->isActive())
						{
							g_engine->onKeyboardKeyDown(SDL_SCANCODE_RETURN);
							g_engine->onKeyboardKeyUp(SDL_SCANCODE_RETURN);
						}
					}
					else if ((env->getOS() == Environment::OS::OS_HORIZON ? events[i].jbutton.button == 10 : events[i].jbutton.button == 7) || events[i].jbutton.button == 1) // KEY_PLUS/KEY_START || KEY_B
						g_engine->onKeyboardKeyDown(SDL_SCANCODE_ESCAPE);
					else if (events[i].jbutton.button == 2) // KEY_X
					{
						g_engine->onKeyboardKeyDown(SDL_SCANCODE_X);
						xDown = true;
					}
					else if (events[i].jbutton.button == 3) // KEY_Y
						g_engine->onKeyboardKeyDown(SDL_SCANCODE_Y);
					else if ((env->getOS() == Environment::OS::OS_HORIZON ? events[i].jbutton.button == 21 : false) || (env->getOS() == Environment::OS::OS_HORIZON ? events[i].jbutton.button == 13 : false)) // right stick up || dpad up
						g_engine->onKeyboardKeyDown(SDL_SCANCODE_UP);
					else if ((env->getOS() == Environment::OS::OS_HORIZON ? events[i].jbutton.button == 23 : false) || (env->getOS() == Environment::OS::OS_HORIZON ? events[i].jbutton.button == 15 : false)) // right stick down || dpad down
						g_engine->onKeyboardKeyDown(SDL_SCANCODE_DOWN);
					else if ((env->getOS() == Environment::OS::OS_HORIZON ? events[i].jbutton.button == 20 : false) || (env->getOS() == Environment::OS::OS_HORIZON ? events[i].jbutton.button == 12 : false)) // right stick left || dpad left
						g_engine->onKeyboardKeyDown(SDL_SCANCODE_LEFT);
					else if ((env->getOS() == Environment::OS::OS_HORIZON ? events[i].jbutton.button == 22 : false) || (env->getOS() == Environment::OS::OS_HORIZON ? events[i].jbutton.button == 14 : false)) // right stick right || dpad right
						g_engine->onKeyboardKeyDown(SDL_SCANCODE_RIGHT);
					else if ((env->getOS() == Environment::OS::OS_HORIZON ? events[i].jbutton.button == 6 : events[i].jbutton.button == 4)) // KEY_L
						g_engine->onKeyboardKeyDown((env->getOS() == Environment::OS::OS_HORIZON ? SDL_SCANCODE_L : SDL_SCANCODE_BACKSPACE));
					else if ((env->getOS() == Environment::OS::OS_HORIZON ? events[i].jbutton.button == 7 : events[i].jbutton.button == 5)) // KEY_R
						g_engine->onKeyboardKeyDown((env->getOS() == Environment::OS::OS_HORIZON ? SDL_SCANCODE_R : SDL_SCANCODE_LSHIFT));
					else if ((env->getOS() == Environment::OS::OS_HORIZON ? events[i].jbutton.button == 8 : false)) // KEY_ZL
						g_engine->onKeyboardKeyDown(SDL_SCANCODE_Z);
					else if ((env->getOS() == Environment::OS::OS_HORIZON ? events[i].jbutton.button == 9 : false)) // KEY_ZR
						g_engine->onKeyboardKeyDown(SDL_SCANCODE_V);
					else if ((env->getOS() == Environment::OS::OS_HORIZON ? events[i].jbutton.button == 11 : events[i].jbutton.button == 6)) // KEY_MINUS/KEY_SELECT
						g_engine->onKeyboardKeyDown(SDL_SCANCODE_F1);
					else if ((env->getOS() == Environment::OS::OS_HORIZON ? events[i].jbutton.button == 4 : events[i].jbutton.button == 9)) // left stick press
					{
						// toggle options (CTRL + O)
						g_engine->onKeyboardKeyDown(SDL_SCANCODE_LCTRL);
						g_engine->onKeyboardKeyDown(SDL_SCANCODE_O);
						g_engine->onKeyboardKeyUp(SDL_SCANCODE_LCTRL);
						g_engine->onKeyboardKeyUp(SDL_SCANCODE_O);
					}
					else if ((env->getOS() == Environment::OS::OS_HORIZON ? events[i].jbutton.button == 5 : events[i].jbutton.button == 10)) // right stick press
					{
						if (xDown)
						{
							// toggle console
							g_engine->onKeyboardKeyDown(SDL_SCANCODE_LSHIFT);
							g_engine->onKeyboardKeyDown(SDL_SCANCODE_F1);
							g_engine->onKeyboardKeyUp(SDL_SCANCODE_LSHIFT);
							g_engine->onKeyboardKeyUp(SDL_SCANCODE_F1);
						}
						else
						{
#ifdef __SWITCH__

							((HorizonSDLEnvironment*)environment)->showKeyboard();

#endif
						}
					}
					break;

				case SDL_EVENT_JOYSTICK_BUTTON_UP :
					if (events[i].jbutton.button == 0) // KEY_A
						g_engine->onMouseLeftChange(false);
					else if ((env->getOS() == Environment::OS::OS_HORIZON ? events[i].jbutton.button == 10 : events[i].jbutton.button == 7) || events[i].jbutton.button == 1) // KEY_PLUS/KEY_START || KEY_B
						g_engine->onKeyboardKeyUp(SDL_SCANCODE_ESCAPE);
					else if (events[i].jbutton.button == 2) // KEY_X
					{
						g_engine->onKeyboardKeyUp(SDL_SCANCODE_X);
						xDown = false;
					}
					else if (events[i].jbutton.button == 3) // KEY_Y
						g_engine->onKeyboardKeyUp(SDL_SCANCODE_Y);
					else if ((env->getOS() == Environment::OS::OS_HORIZON ? events[i].jbutton.button == 21 : false) || (env->getOS() == Environment::OS::OS_HORIZON ? events[i].jbutton.button == 13 : false)) // right stick up || dpad up
						g_engine->onKeyboardKeyUp(SDL_SCANCODE_UP);
					else if ((env->getOS() == Environment::OS::OS_HORIZON ? events[i].jbutton.button == 23 : false) || (env->getOS() == Environment::OS::OS_HORIZON ? events[i].jbutton.button == 15 : false)) // right stick down || dpad down
						g_engine->onKeyboardKeyUp(SDL_SCANCODE_DOWN);
					else if ((env->getOS() == Environment::OS::OS_HORIZON ? events[i].jbutton.button == 20 : false) || (env->getOS() == Environment::OS::OS_HORIZON ? events[i].jbutton.button == 12 : false)) // right stick left || dpad left
						g_engine->onKeyboardKeyUp(SDL_SCANCODE_LEFT);
					else if ((env->getOS() == Environment::OS::OS_HORIZON ? events[i].jbutton.button == 22 : false) || (env->getOS() == Environment::OS::OS_HORIZON ? events[i].jbutton.button == 14 : false)) // right stick right || dpad right
						g_engine->onKeyboardKeyUp(SDL_SCANCODE_RIGHT);
					else if ((env->getOS() == Environment::OS::OS_HORIZON ? events[i].jbutton.button == 6 : events[i].jbutton.button == 4)) // KEY_L
						g_engine->onKeyboardKeyUp((env->getOS() == Environment::OS::OS_HORIZON ? SDL_SCANCODE_L : SDL_SCANCODE_BACKSPACE));
					else if ((env->getOS() == Environment::OS::OS_HORIZON ? events[i].jbutton.button == 7 : events[i].jbutton.button == 5)) // KEY_R
						g_engine->onKeyboardKeyUp((env->getOS() == Environment::OS::OS_HORIZON ? SDL_SCANCODE_R : SDL_SCANCODE_LSHIFT));
					else if ((env->getOS() == Environment::OS::OS_HORIZON ? events[i].jbutton.button == 8 : false)) // KEY_ZL
						g_engine->onKeyboardKeyUp(SDL_SCANCODE_Z);
					else if ((env->getOS() == Environment::OS::OS_HORIZON ? events[i].jbutton.button == 9 : false)) // KEY_ZR
						g_engine->onKeyboardKeyUp(SDL_SCANCODE_V);
					else if ((env->getOS() == Environment::OS::OS_HORIZON ? events[i].jbutton.button == 11 : events[i].jbutton.button == 6)) // KEY_MINUS/KEY_SELECT
						g_engine->onKeyboardKeyUp(SDL_SCANCODE_F1);
					break;

				case SDL_EVENT_JOYSTICK_AXIS_MOTION :
					//debugLog("joyaxismotion: stick %i : axis = %i, value = %i\n", (int)events[i].jaxis.which, (int)events[i].jaxis.axis, (int)events[i].jaxis.value);
					// left stick
					if (events[i].jaxis.axis == 1 || events[i].jaxis.axis == 0)
					{
						if (events[i].jaxis.axis == 0)
							m_fJoystick0XPercent = clamp<float>((float)events[i].jaxis.value / 32767.0f, -1.0f, 1.0f);
						else
							m_fJoystick0YPercent = clamp<float>((float)events[i].jaxis.value / 32767.0f, -1.0f, 1.0f);
					}
					if (env->getOS() != Environment::OS::OS_HORIZON)
					{
						// ZL/ZR
						if (events[i].jaxis.axis == 2 || events[i].jaxis.axis == 5)
						{
							if (events[i].jaxis.axis == 2)
							{
								const float threshold = sdl_joystick_zl_threshold.getFloat();
								const float percent = clamp<float>((float)events[i].jaxis.value / 32767.0f, -1.0f, 1.0f);
								const bool wasZlDown = zlDown;
								zlDown = !(threshold <= 0.0f ? percent <= threshold : percent >= threshold);
								if (zlDown != wasZlDown)
								{
									if (zlDown)
										g_engine->onKeyboardKeyDown(SDL_SCANCODE_KP_MINUS);
									else
										g_engine->onKeyboardKeyUp(SDL_SCANCODE_KP_MINUS);
								}
							}
							else
							{
								const float threshold = sdl_joystick_zr_threshold.getFloat();
								const float percent = clamp<float>((float)events[i].jaxis.value / 32767.0f, -1.0f, 1.0f);
								const bool wasZrDown = zrDown;
								zrDown = !(threshold <= 0.0f ? percent <= threshold : percent >= threshold);
								if (zrDown != wasZrDown)
								{
									if (zrDown)
										g_engine->onKeyboardKeyDown(SDL_SCANCODE_KP_PLUS);
									else
										g_engine->onKeyboardKeyUp(SDL_SCANCODE_KP_PLUS);
								}
							}
						}
					}
					break;

				case SDL_EVENT_JOYSTICK_HAT_MOTION :
					//debugLog("joyhatmotion: hat %i : value = %i\n", (int)events[i].jhat.hat, (int)events[i].jhat.value);
					if (env->getOS() != Environment::OS::OS_HORIZON)
					{
						const bool wasHatUpDown = hatUpDown;
						const bool wasHatDownDown = hatDownDown;
						const bool wasHatLeftDown = hatLeftDown;
						const bool wasHatRightDown = hatRightDown;

						hatUpDown = (events[i].jhat.value == SDL_HAT_UP);
						hatDownDown = (events[i].jhat.value == SDL_HAT_DOWN);
						hatLeftDown = (events[i].jhat.value == SDL_HAT_LEFT);
						hatRightDown = (events[i].jhat.value == SDL_HAT_RIGHT);

						if (hatUpDown != wasHatUpDown)
						{
							if (hatUpDown)
								g_engine->onKeyboardKeyDown(SDL_SCANCODE_UP);
							else
								g_engine->onKeyboardKeyUp(SDL_SCANCODE_UP);
						}

						if (hatDownDown != wasHatDownDown)
						{
							if (hatDownDown)
								g_engine->onKeyboardKeyDown(SDL_SCANCODE_DOWN);
							else
								g_engine->onKeyboardKeyUp(SDL_SCANCODE_DOWN);
						}

						if (hatLeftDown != wasHatLeftDown)
						{
							if (hatLeftDown)
								g_engine->onKeyboardKeyDown(SDL_SCANCODE_LEFT);
							else
								g_engine->onKeyboardKeyUp(SDL_SCANCODE_LEFT);
						}

						if (hatRightDown != wasHatRightDown)
						{
							if (hatRightDown)
								g_engine->onKeyboardKeyDown(SDL_SCANCODE_RIGHT);
							else
								g_engine->onKeyboardKeyUp(SDL_SCANCODE_RIGHT);
						}
					}
					break;

#endif
				}
			}
			} while (eventCount == SIZE_EVENTS);
		}

		// update
		{
			deltaTimer->update();
			g_engine->setFrameTime(deltaTimer->getDelta());

#if defined(MCENGINE_SDL_JOYSTICK) && defined(MCENGINE_SDL_JOYSTICK_MOUSE)

			// joystick mouse
			{
				// apply deadzone
				float joystick0XPercent = m_fJoystick0XPercent;
				float joystick0YPercent = m_fJoystick0YPercent;
				{
					const float joystick0DeadzoneX = sdl_joystick0_deadzone.getFloat();
					const float joystick0DeadzoneY = sdl_joystick0_deadzone.getFloat();

					if (joystick0DeadzoneX > 0.0f && joystick0DeadzoneX < 1.0f)
					{
						const float deltaAbs = (std::abs(m_fJoystick0XPercent) - joystick0DeadzoneX);
						joystick0XPercent = (deltaAbs > 0.0f ? (deltaAbs / (1.0f - joystick0DeadzoneX)) * (float)signbit(m_fJoystick0XPercent) : 0.0f);
					}

					if (joystick0DeadzoneY > 0.0f && joystick0DeadzoneY < 1.0f)
					{
						const float deltaAbs = (std::abs(m_fJoystick0YPercent) - joystick0DeadzoneY);
						joystick0YPercent = (deltaAbs > 0.0f ? (deltaAbs / (1.0f - joystick0DeadzoneY)) * (float)signbit(m_fJoystick0YPercent) : 0.0f);
					}
				}

				if (g_bHasFocus && !g_bMinimized && (joystick0XPercent != 0.0f || joystick0YPercent != 0.0f))
				{
					const float hardcodedMultiplier = 1000.0f;
					const Vector2 hardcodedResolution = Vector2(1280, 720);
					Vector2 joystickDelta = Vector2(joystick0XPercent * sdl_joystick_mouse_sensitivity.getFloat(), joystick0YPercent * sdl_joystick_mouse_sensitivity.getFloat()) * g_engine->getFrameTime() * hardcodedMultiplier;
					joystickDelta *= g_engine->getScreenSize().x/hardcodedResolution.x > g_engine->getScreenSize().y/hardcodedResolution.y ?
									 g_engine->getScreenSize().y/hardcodedResolution.y : g_engine->getScreenSize().x/hardcodedResolution.x; // normalize

					mousePos += joystickDelta;
					mousePos.x = clamp<float>(mousePos.x, 0.0f, g_engine->getScreenSize().x - 1);
					mousePos.y = clamp<float>(mousePos.y, 0.0f, g_engine->getScreenSize().y - 1);

					environment->setWasLastMouseInputTouch(false);

					environment->setMousePos(mousePos.x, mousePos.y);
					g_engine->getMouse()->onPosChange(mousePos);
				}
			}
#endif

			if (g_bUpdate)
				g_engine->onUpdate();
		}

		// draw
		if (g_bDraw)
		{
			g_bDrawing = true;
			{
				g_engine->onPaint();
			}
			g_bDrawing = false;
		}

		// delay the next frame (if desired)
		{
			VPROF_BUDGET("FPSLimiter", VPROF_BUDGETGROUP_SLEEP);

			const bool inBackground = g_bMinimized || !g_bHasFocus;
			const bool shouldSleep = (!fps_unlimited.getBool() && fps_max.getFloat() > 0) || inBackground;
			const bool shouldYield = fps_unlimited_yield.getBool() || fps_max_yield.getBool();

			if (shouldSleep)
			{
				const float targetFps = inBackground ? fps_max_background.getFloat() : fps_max.getFloat();

				frameCountSinceLastFpsCalc++;
				fpsCalcTimer->update();

				if (fpsCalcTimer->getElapsedTime() >= 0.5f) {
					if (!inBackground)
					{
						const double actualGameFps = static_cast<double>(frameCountSinceLastFpsCalc) / fpsCalcTimer->getElapsedTime();
						if (actualGameFps < targetFps * 0.99f && actualGameFps > targetFps * 0.85f)
							fpsAdjustment -= 0.5f;
						else if (actualGameFps > targetFps * 1.005f)
							fpsAdjustment += 0.5f;
						fpsAdjustment = clamp<double>(fpsAdjustment, -15.0f, 0.0f);
					}
					else fpsAdjustment = 0.0f;

					// reset fps adjustment timer for the next measurement period
					frameCountSinceLastFpsCalc = 0;
					fpsCalcTimer->start();
				}
				frameTimer->update();
				const double frameTimeDelta = frameTimer->getDelta();
				const double adjustedTargetFrameTime = (1.0f / targetFps) * (1.0f + (inBackground ? 0.0f : fpsAdjustment) / 100.0f);

				// finally sleep for the adjusted amount of time
				if (frameTimeDelta < adjustedTargetFrameTime)
					env->sleep(static_cast<unsigned int>((adjustedTargetFrameTime - frameTimeDelta) * SDL_US_PER_SECOND));
				else if (shouldYield)
					env->sleep(0);

				// set the start time of the next loop iteration
				frameTimer->update();
			}
			else if (shouldYield) env->sleep(0);
		}
	}

	// release the timers
	SAFE_DELETE(frameTimer);
	SAFE_DELETE(deltaTimer);
	SAFE_DELETE(fpsCalcTimer);

	// release engine
    SAFE_DELETE(g_engine);

    // and the opengl context
#if defined(MCENGINE_FEATURE_OPENGL) || defined(MCENGINE_FEATURE_OPENGLES)

	SDL_GL_DestroyContext(context);

#endif

	SDL_StopTextInput(g_window);

	// finally, destroy the window
	SDL_DestroyWindow(g_window);
	SDL_Quit();

	// TODO: handle potential restart

	return 0;
}



int mainSDL(int argc, char *argv[])
{
	return mainSDL(argc, argv, NULL);
}

#endif
