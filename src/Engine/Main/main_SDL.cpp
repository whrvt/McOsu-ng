//========== Copyright (c) 2018, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		main SDL entry point
//
// $NoKeywords: $mainSDL
//===============================================================================//

#include "EngineFeatures.h"

#ifdef MCENGINE_FEATURE_SDL

#if !defined(MCENGINE_FEATURE_OPENGL) && !defined(MCENGINE_FEATURE_GLES2) && !defined(MCENGINE_FEATURE_GLES32)
#error OpenGL support is currently required for SDL
#endif

#include "Profiler.h"
#include "Timer.h"

#include "HorizonSDLEnvironment.h"
#include "SDLEnvironment.h"
#include "WinSDLEnvironment.h"

static constexpr auto WINDOW_TITLE = "McEngine"; // only the initial title

static constexpr auto WINDOW_WIDTH = 1280L;
static constexpr auto WINDOW_HEIGHT = 720L;

static constexpr auto WINDOW_WIDTH_MIN = 100;
static constexpr auto WINDOW_HEIGHT_MIN = 100;

ConVar fps_max("fps_max", 60.0f, FCVAR_NONE, "framerate limiter, foreground");
ConVar fps_max_yield("fps_max_yield", true, FCVAR_NONE, "always release rest of timeslice once per frame (call scheduler via sleep(0))");
ConVar fps_max_background("fps_max_background", 30.0f, FCVAR_NONE, "framerate limiter, background");
ConVar fps_unlimited("fps_unlimited", false, FCVAR_NONE);
ConVar fps_unlimited_yield("fps_unlimited_yield", true, FCVAR_NONE, "always release rest of timeslice once per frame (call scheduler via sleep(0)), even if unlimited fps are enabled");

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

[[maybe_unused]] static void dumpSDLGLAttribs()
{
	int value;
	const char *attrNames[] = {"SDL_GL_RED_SIZE",
	                           "SDL_GL_GREEN_SIZE",
	                           "SDL_GL_BLUE_SIZE",
	                           "SDL_GL_ALPHA_SIZE",
	                           "SDL_GL_BUFFER_SIZE",
	                           "SDL_GL_DOUBLEBUFFER",
	                           "SDL_GL_DEPTH_SIZE",
	                           "SDL_GL_STENCIL_SIZE",
	                           "SDL_GL_ACCUM_RED_SIZE",
	                           "SDL_GL_ACCUM_GREEN_SIZE",
	                           "SDL_GL_ACCUM_BLUE_SIZE",
	                           "SDL_GL_ACCUM_ALPHA_SIZE",
	                           "SDL_GL_STEREO",
	                           "SDL_GL_MULTISAMPLEBUFFERS",
	                           "SDL_GL_MULTISAMPLESAMPLES",
	                           "SDL_GL_ACCELERATED_VISUAL",
	                           "SDL_GL_RETAINED_BACKING",
	                           "SDL_GL_CONTEXT_MAJOR_VERSION",
	                           "SDL_GL_CONTEXT_MINOR_VERSION",
	                           "SDL_GL_CONTEXT_FLAGS",
	                           "SDL_GL_CONTEXT_PROFILE_MASK",
	                           "SDL_GL_SHARE_WITH_CURRENT_CONTEXT",
	                           "SDL_GL_FRAMEBUFFER_SRGB_CAPABLE",
	                           "SDL_GL_CONTEXT_RELEASE_BEHAVIOR",
	                           "SDL_GL_CONTEXT_RESET_NOTIFICATION",
	                           "SDL_GL_CONTEXT_NO_ERROR",
	                           "SDL_GL_FLOATBUFFERS",
	                           "SDL_GL_EGL_PLATFORM"};

	debugLog("=== SDL OpenGL Attributes ===\n");

	// Iterate through all attributes
	for (int attr = SDL_GL_RED_SIZE; attr <= SDL_GL_EGL_PLATFORM; attr++)
	{
		if (SDL_GL_GetAttribute((SDL_GLAttr)attr, &value))
		{
			debugLog("%s = %d\n", attrNames[attr], value);
		}
		else
		{
			debugLog("%s = <error: %s>\n", attrNames[attr], SDL_GetError());
		}
	}

	debugLog("===========================\n");
}

int SDLEnvironment::main(int argc, char *argv[])
{
	constexpr auto flags = SDL_INIT_VIDEO | (Env::cfg(FEAT::JOY | FEAT::JOY_MOU) ? SDL_INIT_JOYSTICK : 0u);

	// initialize sdl
	if (!SDL_Init(flags))
	{
		debugLog("Couldn't SDL_Init(): %s\n", SDL_GetError());
		return 1;
	}

	SDL_SetLogOutputFunction(SDLLogCallback, nullptr);

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
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_BORDERLESS_BOOLEAN, false); // set these props later, after engine starts and loads cfgs
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_FULLSCREEN_BOOLEAN, false);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_MAXIMIZED_BOOLEAN, false);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_FLAGS_NUMBER, windowFlags);

	// create window
	m_window = SDL_CreateWindowWithProperties(props);
	SDL_DestroyProperties(props);

	if (m_window == NULL)
	{
		debugLog("Couldn't SDL_CreateWindow(): %s\n", SDL_GetError());
		return 1;
	}

	// get the screen refresh rate, and set fps_max to that as default
	{
		const SDL_DisplayMode *currentDisplayMode;
		const SDL_DisplayID display = SDL_GetDisplayForWindow(m_window);
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
	SDL_GLContext context;
	if constexpr (Env::cfg((REND::GL | REND::GLES2 | REND::GLES32), !REND::DX11))
	{
		context = SDL_GL_CreateContext(m_window);
		SDL_GL_MakeCurrent(m_window, context);
	}

	if constexpr (Env::cfg(FEAT::JOY | FEAT::JOY_MOU))
	{
		SDL_OpenJoystick(0);
		SDL_OpenJoystick(1);
	}
	else
	{
		SDL_SetEventEnabled(SDL_EVENT_JOYSTICK_AXIS_MOTION, false);
		SDL_SetEventEnabled(SDL_EVENT_JOYSTICK_BALL_MOTION, false);
		SDL_SetEventEnabled(SDL_EVENT_JOYSTICK_HAT_MOTION, false);
		SDL_SetEventEnabled(SDL_EVENT_JOYSTICK_BUTTON_DOWN, false);
		SDL_SetEventEnabled(SDL_EVENT_JOYSTICK_BUTTON_UP, false);
		SDL_SetEventEnabled(SDL_EVENT_JOYSTICK_ADDED, false);
		SDL_SetEventEnabled(SDL_EVENT_JOYSTICK_REMOVED, false);
		SDL_SetEventEnabled(SDL_EVENT_JOYSTICK_BATTERY_UPDATED, false);
		SDL_SetEventEnabled(SDL_EVENT_JOYSTICK_UPDATE_COMPLETE, false);
	}

	// TODO: add SDL pen support
	{
		SDL_SetEventEnabled(SDL_EVENT_PEN_PROXIMITY_IN, false);
		SDL_SetEventEnabled(SDL_EVENT_PEN_PROXIMITY_OUT, false);
		SDL_SetEventEnabled(SDL_EVENT_PEN_DOWN, false);
		SDL_SetEventEnabled(SDL_EVENT_PEN_UP, false);
		SDL_SetEventEnabled(SDL_EVENT_PEN_BUTTON_DOWN, false);
		SDL_SetEventEnabled(SDL_EVENT_PEN_BUTTON_UP, false);
		SDL_SetEventEnabled(SDL_EVENT_PEN_MOTION, false);
		SDL_SetEventEnabled(SDL_EVENT_PEN_AXIS, false);
	}

	if constexpr (!Env::cfg(FEAT::TOUCH))
	{
		SDL_SetEventEnabled(SDL_EVENT_FINGER_DOWN, false);
		SDL_SetEventEnabled(SDL_EVENT_FINGER_UP, false);
		SDL_SetEventEnabled(SDL_EVENT_FINGER_MOTION, false);
		SDL_SetEventEnabled(SDL_EVENT_FINGER_CANCELED, false);
	}

	// create timers
	auto *frameTimer = new Timer();
	auto *deltaTimer = new Timer();
	auto *fpsCalcTimer = new Timer();

	// variables to keep track of fps overhead adjustment
	uint64_t frameCountSinceLastFpsCalc = 0;
	double fpsAdjustment = 0.0;

	// initialize engine
	m_engine = new Engine(this, argc > 1 ? argv[1] : ""); // TODO: proper arg support

	// post window-creation settings
	SDL_SetWindowMinimumSize(m_window, WINDOW_WIDTH_MIN, WINDOW_HEIGHT_MIN);

	m_engine->loadApp();

	frameTimer->update();
	deltaTimer->update();
	fpsCalcTimer->update();

	if constexpr (Env::cfg((REND::GL | REND::GLES2 | REND::GLES32), !REND::DX11))
	{
		if (sdlDebug()) // TODO: the variable somehow isn't loaded from the engine config at this point yet, so this is unreachable
			dumpSDLGLAttribs();
	}

	const bool shouldBeBorderless = convar->getConVarByName("fullscreen_windowed_borderless")->getBool();
	const bool shouldBeFullscreen = !(convar->getConVarByName("windowed") != NULL) || shouldBeBorderless;

	if (!(shouldBeBorderless || shouldBeFullscreen))
	{
		disableFullscreen();
		center();
	}
	else
	{
		enableFullscreen();
	}

	// make the window visible
	SDL_ShowWindow(m_window);
	SDL_RaiseWindow(m_window);

	// sdl3 stops listening to text input globally when window is created
	SDL_StartTextInput(m_window);
	SDL_SetWindowKeyboardGrab(m_window, false); // this allows windows key and such to work, listenToTextInput will set/unset the keyboard grab when necessary

	Vector2 mousePos;

	// main loop
	constexpr auto SIZE_EVENTS = 64u;
	SDL_Event events[SIZE_EVENTS];
	while (m_bRunning)
	{
		VPROF_MAIN();

		// HACKHACK: switch hack (usb mouse/keyboard support)
		// if constexpr (Env::cfg(OS::HORIZON))
		// {
		// 	HorizonSDLEnvironment *horizonSDLenv = dynamic_cast<HorizonSDLEnvironment*>(environment);
		// 	if (horizonSDLenv != NULL)
		// 		horizonSDLenv->update_before_winproc();
		// }

		// handle window message queue
		{
			VPROF_BUDGET("SDL", VPROF_BUDGETGROUP_WNDPROC);

			// handle automatic raw input toggling
			const bool isDebugSdl = sdlDebug();
			const bool isRawInputEnabled = doStupidRawinputLogicCheck();

			SDL_PumpEvents();
			auto eventCount = 0u;
			do
			{
				eventCount = SDL_PeepEvents(&events[0], SIZE_EVENTS, SDL_GETEVENT, SDL_EVENT_FIRST, SDL_EVENT_LAST);
				// debugLog("eventCount: %d\n", eventCount);
				for (auto i = 0u; i < eventCount; ++i)
				{
					switch (events[i].type)
					{
					case SDL_EVENT_QUIT:
						if (m_bRunning)
						{
							m_bRunning = false;
							m_engine->onShutdown();
						}
						break;

					// window
					case SDL_EVENT_WINDOW_FIRST ... SDL_EVENT_WINDOW_LAST: {
						switch (events[i].window.type)
						{
						case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
							if (m_bRunning)
							{
								m_bRunning = false;
								m_engine->onShutdown();
							}
							break;
						case SDL_EVENT_WINDOW_FOCUS_GAINED:
							m_bHasFocus = true;
							m_engine->onFocusGained();
							break;
						case SDL_EVENT_WINDOW_FOCUS_LOST:
							m_bHasFocus = false;
							m_engine->onFocusLost();
							break;
						case SDL_EVENT_WINDOW_MAXIMIZED:
							m_bMinimized = false;
							m_engine->onMaximized();
							break;
						case SDL_EVENT_WINDOW_MINIMIZED:
							m_bMinimized = true;
							m_engine->onMinimized();
							break;
						case SDL_EVENT_WINDOW_RESTORED:
							m_bMinimized = false;
							m_engine->onRestored();
							break;
						case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
						case SDL_EVENT_WINDOW_RESIZED:
							m_engine->requestResolutionChange(Vector2(events[i].window.data1, events[i].window.data2));
							break;
						default:
							// debugLog("DEBUG: unhandled window event: %#08x\n", events[i].window.type);
							break;
						}
						break;
					}
					// keyboard
					case SDL_EVENT_KEY_DOWN:
						// debugLog("DEBUG: keydown event: %#08x\n", events[i].key.scancode);
						m_engine->onKeyboardKeyDown(events[i].key.scancode);
						break;

					case SDL_EVENT_KEY_UP:
						// debugLog("DEBUG: keyup event: %#08x\n", events[i].key.scancode);
						m_engine->onKeyboardKeyUp(events[i].key.scancode);
						break;

					case SDL_EVENT_TEXT_INPUT:
						// debugLog("DEBUG: text input event: %s\n", events[i].text.text);
						{
							UString nullTerminatedTextInputString(events[i].text.text);
							for (int i = 0; i < nullTerminatedTextInputString.length(); i++)
							{
								m_engine->onKeyboardChar((KEYCODE)nullTerminatedTextInputString[i]); // NOTE: this splits into UTF-16 wchar_t atm
							}
						}
						break;

					// mouse
					case SDL_EVENT_MOUSE_BUTTON_DOWN:
						if (likely(!deckTouchHack())) // HACKHACK: Steam Deck workaround (sends mouse events even though native touchscreen support is enabled)
						{
							switch (events[i].button.button)
							{
							case SDL_BUTTON_LEFT:
								m_engine->onMouseLeftChange(true);
								break;
							case SDL_BUTTON_MIDDLE:
								m_engine->onMouseMiddleChange(true);
								break;
							case SDL_BUTTON_RIGHT:
								m_engine->onMouseRightChange(true);
								break;

							case SDL_BUTTON_X1:
								m_engine->onMouseButton4Change(true);
								break;
							case SDL_BUTTON_X2:
								m_engine->onMouseButton5Change(true);
								break;
							}
						}
						break;

					case SDL_EVENT_MOUSE_BUTTON_UP:
						if (likely(!deckTouchHack())) // HACKHACK: Steam Deck workaround (sends mouse events even though native touchscreen support is enabled)
						{
							switch (events[i].button.button)
							{
							case SDL_BUTTON_LEFT:
								m_engine->onMouseLeftChange(false);
								break;
							case SDL_BUTTON_MIDDLE:
								m_engine->onMouseMiddleChange(false);
								break;
							case SDL_BUTTON_RIGHT:
								m_engine->onMouseRightChange(false);
								break;

							case SDL_BUTTON_X1:
								m_engine->onMouseButton4Change(false);
								break;
							case SDL_BUTTON_X2:
								m_engine->onMouseButton5Change(false);
								break;
							}
						}
						break;

					case SDL_EVENT_MOUSE_WHEEL:
						if (events[i].wheel.x != 0)
							m_engine->onMouseWheelHorizontal(events[i].wheel.x > 0 ? 120 * std::abs(events[i].wheel.x) : -120 * std::abs(events[i].wheel.x)); // NOTE: convert to Windows units
						if (events[i].wheel.y != 0)
							m_engine->onMouseWheelVertical(events[i].wheel.y > 0 ? 120 * std::abs(events[i].wheel.y) : -120 * std::abs(events[i].wheel.y)); // NOTE: convert to Windows units
						break;

					case SDL_EVENT_MOUSE_MOTION:
						if (likely(!deckTouchHack())) // HACKHACK: Steam Deck workaround (sends mouse events even though native touchscreen support is enabled)
						{
							if constexpr (Env::cfg(FEAT::TOUCH))
								if (events[i].motion.which != SDL_TOUCH_MOUSEID)
									setWasLastMouseInputTouch(false);

							if (unlikely(isDebugSdl))
								debugLog("SDL_MOUSEMOTION: x = %.2f, xrel = %.2f, y = %.2f, yrel = %.2f, which = %i\n", events[i].motion.x, events[i].motion.xrel, events[i].motion.y, events[i].motion.yrel, (int)events[i].motion.which);

							// which!=0 means relative, ignore non-relative motion events if we want raw input
							if (isRawInputEnabled == !!events[i].motion.which)
							{
								// store the position for future queries
								m_vLastAbsMousePos.x = events[i].motion.x;
								m_vLastAbsMousePos.y = events[i].motion.y;

								m_vLastRelMousePos.x = events[i].motion.xrel; // TODO: use?
								m_vLastRelMousePos.y = events[i].motion.yrel;
							}
							if (isRawInputEnabled)
								m_engine->onMouseRawMove(events[i].motion.xrel, events[i].motion.yrel);
						}
						break;

					// touch mouse
					// NOTE: sometimes when quickly tapping with two fingers, events will get lost (due to the touchscreen believing that it was one finger which moved very quickly, instead of 2 tapping fingers)
					case SDL_EVENT_FINGER_DOWN ... SDL_EVENT_FINGER_CANCELED:
						if constexpr (Env::cfg(FEAT::TOUCH))
							handleTouchEvent(events[i], events[i].type, &mousePos);
						break;

					// joystick keyboard
					// NOTE: defaults to xbox 360 controller layout on all non-horizon environments
					case SDL_EVENT_JOYSTICK_AXIS_MOTION ... SDL_EVENT_JOYSTICK_UPDATE_COMPLETE:
						if constexpr (Env::cfg(FEAT::JOY))
							handleJoystickEvent(events[i], events[i].type);
						break;
					}
				}
			} while (eventCount == SIZE_EVENTS);
		}

		// update
		{
			deltaTimer->update();
			m_engine->setFrameTime(deltaTimer->getDelta());

			if constexpr (Env::cfg(FEAT::JOY_MOU))
				handleJoystickMouse(&mousePos);

			if constexpr (m_bUpdate)
				m_engine->onUpdate();
		}

		// draw
		if constexpr (m_bDraw) // the only reason this even exists is because of copy-pasting, windows main doesn't draw when minimized
		{
			m_bDrawing = true;
			{
				m_engine->onPaint();
			}
			m_bDrawing = false;
		}

		// delay the next frame (if desired)
		{
			VPROF_BUDGET("FPSLimiter", VPROF_BUDGETGROUP_SLEEP);

			const bool inBackground = m_bMinimized || !m_bHasFocus;
			const bool shouldSleep = inBackground || (!fps_unlimited.getBool() && fps_max.getFloat() > 0);
			const bool shouldYield = fps_unlimited_yield.getBool() || fps_max_yield.getBool();

			if (shouldSleep)
			{
				const float targetFps = inBackground ? fps_max_background.getFloat() : fps_max.getFloat();

				frameCountSinceLastFpsCalc++;
				fpsCalcTimer->update();

				if (fpsCalcTimer->getElapsedTime() >= 0.5f)
				{
					if (!inBackground)
					{
						const double actualGameFps = static_cast<double>(frameCountSinceLastFpsCalc) / fpsCalcTimer->getElapsedTime();
						if (actualGameFps < targetFps * 0.99f && actualGameFps > targetFps * 0.85f)
							fpsAdjustment -= 0.5f;
						else if (actualGameFps > targetFps * 1.005f)
							fpsAdjustment += 0.5f;
						fpsAdjustment = clamp<double>(fpsAdjustment, -15.0f, 0.0f);
					}
					else
						fpsAdjustment = 0.0f;

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
			else if (shouldYield)
				env->sleep(0);
		}
	}

	// release the timers
	SAFE_DELETE(frameTimer);
	SAFE_DELETE(deltaTimer);
	SAFE_DELETE(fpsCalcTimer);

	// and the opengl context
	if constexpr (Env::cfg((REND::GL | REND::GLES2 | REND::GLES32), !REND::DX11))
		SDL_GL_DestroyContext(context);

	SDL_StopTextInput(m_window);

	// finally, destroy the window
	SDL_DestroyWindow(m_window);
	SDL_Quit();

	// TODO: handle potential restart

	return 0;
}

#endif
