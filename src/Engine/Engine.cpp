//================ Copyright (c) 2012, PG, All rights reserved. =================//
//
// Purpose:		core
//
// $NoKeywords: $engine
//===============================================================================//

#include <cstdio>

#include "AnimationHandler.h"
#include "ConVar.h"
#include "DiscordInterface.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "NetworkHandler.h"
#include "Profiler.h"
#include "ResourceManager.h"
#include "SoundEngine.h"
#include "SteamworksInterface.h"
#include "Timing.h"

#include "CBaseUIContainer.h"

#include "Console.h"
#include "ConsoleBox.h"
#include "VisualProfiler.h"

#include "Engine.h"
#include "McMath.h"

void _version(void);
void _host_timescale_(UString oldValue, UString newValue);
ConVar host_timescale("host_timescale", 1.0f, FCVAR_CHEAT, "Scale by which the engine measures elapsed time, affects engine->getTime()", _host_timescale_);
void _host_timescale_(UString oldValue, UString newValue)
{
	if (newValue.toFloat() < 0.01f)
	{
		debugLog(0xffff4444, "Value must be >= 0.01!\n");
		host_timescale.setValue(1.0f);
	}
}
ConVar epilepsy("epilepsy", false, FCVAR_NONE);
ConVar debug_engine("debug_engine", false, FCVAR_NONE);
ConVar minimize_on_focus_lost_if_fullscreen("minimize_on_focus_lost_if_fullscreen", true, FCVAR_NONE);
ConVar minimize_on_focus_lost_if_borderless_windowed_fullscreen("minimize_on_focus_lost_if_borderless_windowed_fullscreen", false, FCVAR_NONE);
ConVar _disable_windows_key("disable_windows_key", false, FCVAR_NONE, "set to 0/1 to disable/enable the Windows/Super key");
ConVar engine_throttle("engine_throttle", true, FCVAR_NONE, "limit some engine component updates to improve performance (non-gameplay-related, only turn this off if you like lower performance for no reason)");

std::unique_ptr<Mouse> Engine::s_mouseInstance = nullptr;
std::unique_ptr<Keyboard> Engine::s_keyboardInstance = nullptr;
std::unique_ptr<App> Engine::s_appInstance = nullptr;
std::unique_ptr<Graphics> Engine::s_graphicsInstance = nullptr;
std::unique_ptr<SoundEngine> Engine::s_soundEngineInstance = nullptr;
std::unique_ptr<ResourceManager> Engine::s_resourceManagerInstance = nullptr;
std::unique_ptr<NetworkHandler> Engine::s_networkHandlerInstance = nullptr;
std::unique_ptr<AnimationHandler> Engine::s_animationHandlerInstance = nullptr;
std::unique_ptr<SteamworksInterface> Engine::s_steamInstance = nullptr;
std::unique_ptr<DiscordInterface> Engine::s_discordInstance = nullptr;

Mouse *mouse = nullptr;
Keyboard *keyboard = nullptr;
App *app = nullptr;
Graphics *graphics = nullptr;
SoundEngine *soundEngine = nullptr;
ResourceManager *resourceManager = nullptr;
NetworkHandler *networkHandler = nullptr;
AnimationHandler *animationHandler = nullptr;
SteamworksInterface *steam = nullptr;
DiscordInterface *discord = nullptr;

Engine *engine = NULL;

Console *Engine::m_console = NULL;
ConsoleBox *Engine::m_consoleBox = NULL;

Engine::Engine()
{
	engine = this;

	m_guiContainer = nullptr;
	m_visualProfiler = nullptr;

	// disable output buffering (else we get multithreading issues due to blocking)
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	// print debug information
	debugLog("-= Engine Startup =-\n");
	_version();
	debugLog("cmdline: {:s}\n", UString::join(env->getCommandLine()).toUtf8());

	// timing
	m_timer = new Timer(false);
	m_dTime = 0;
	m_dRunTime = 0;
	m_iFrameCount = 0;
	m_iVsyncFrameCount = 0;
	m_fVsyncFrameCounterTime = 0.0f;
	m_dFrameTime = 0.016;

	engine_throttle.setCallback(fastdelegate::MakeDelegate(this, &Engine::onEngineThrottleChanged));

	// window
	m_bBlackout = false;
	m_bHasFocus = false;
	m_bIsMinimized = false;

	// screen
	m_bResolutionChange = false;
	m_vScreenSize = env->getWindowSize();
	m_vNewScreenSize = m_vScreenSize;
	m_screenRect = {
	    {0, 0},
        m_vScreenSize
    };

	debugLog("Engine: ScreenSize = ({}x{})\n", (int)m_vScreenSize.x, (int)m_vScreenSize.y);

	// custom
	m_bDrawing = false;
	m_bIsRestarting = false;

	// math (its own singleton)
	m_math = new McMath();

	// initialize all engine subsystems (the order does matter!)
	// add assertions that each subsystem is initialized, so that the constructor (Environment) can
	// safely dereference the global object refs
	debugLog("\nEngine: Initializing subsystems ...\n");
	{
		// input devices
		s_mouseInstance = std::make_unique<Mouse>();
		mouse = s_mouseInstance.get();
		runtime_assert(mouse, "Mouse failed to initialize!");
		m_inputDevices.push_back(mouse);
		m_mice.push_back(mouse);

		s_keyboardInstance = std::make_unique<Keyboard>();
		keyboard = s_keyboardInstance.get();
		runtime_assert(keyboard, "Keyboard failed to initialize!");
		m_inputDevices.push_back(keyboard);
		m_keyboards.push_back(keyboard);

		// create graphics through environment
		graphics = env->createRenderer();
		{
			graphics->init(); // needs init() separation due to potential graphics access
		}
		runtime_assert(graphics, "Graphics failed to initialize!");
		s_graphicsInstance.reset(graphics);

		// make unique_ptrs for the rest
		s_resourceManagerInstance = std::make_unique<ResourceManager>();
		resourceManager = s_resourceManagerInstance.get();
		runtime_assert(resourceManager, "Resource manager menu failed to initialize!");

		s_soundEngineInstance.reset(SoundEngine::createSoundEngine());
		soundEngine = s_soundEngineInstance.get();
		runtime_assert(soundEngine, "Sound engine failed to initialize!");

		s_animationHandlerInstance = std::make_unique<AnimationHandler>();
		animationHandler = s_animationHandlerInstance.get();
		runtime_assert(animationHandler, "Animation handler failed to initialize!");

		s_networkHandlerInstance = std::make_unique<NetworkHandler>();
		networkHandler = s_networkHandlerInstance.get();
		runtime_assert(networkHandler, "Network handler failed to initialize!");

		if constexpr (Env::cfg(FEAT::STEAM))
		{
			s_steamInstance = std::make_unique<SteamworksInterface>();
			steam = s_steamInstance.get();
			runtime_assert(steam, "Steam integration failed to initialize!");
		}

		s_discordInstance = std::make_unique<DiscordInterface>(); // TODO: allow disabling
		discord = s_discordInstance.get();
		runtime_assert(discord, "Discord integration failed to initialize!");

		// default launch overrides
		graphics->setVSync(false);

		// engine time starts now
		m_timer->start();
	}
	debugLog("Engine: Initializing subsystems done.\n\n");
}

Engine::~Engine()
{
	debugLog("\n-= Engine Shutdown =-\n");

	// reset() all static unique_ptrs
	debugLog("Engine: Freeing app...\n");
	s_appInstance.reset();

	if (m_console != NULL)
		showMessageErrorFatal("Engine Error", "m_console not set to NULL before shutdown!");

	debugLog("Engine: Freeing engine GUI...\n");
	{
		m_console = NULL;
		m_consoleBox = NULL;
	}
	SAFE_DELETE(m_guiContainer);

	debugLog("Engine: Freeing resource manager...\n");
	s_resourceManagerInstance.reset();

	debugLog("Engine: Freeing Sound...\n");
	s_soundEngineInstance.reset();

	debugLog("Engine: Freeing animation handler...\n");
	s_animationHandlerInstance.reset();

	debugLog("Engine: Freeing network handler...\n");
	s_networkHandlerInstance.reset();

	if constexpr (Env::cfg(FEAT::STEAM))
	{
		debugLog("Engine: Freeing Steam...\n");
		s_steamInstance.reset();
	}

	debugLog("Engine: Freeing Discord...\n");
	s_discordInstance.reset();

	debugLog("Engine: Freeing input devices...\n");
	// first remove the mouse and keyboard from the input devices
	m_inputDevices.erase(std::remove_if(m_inputDevices.begin(), m_inputDevices.end(), [](InputDevice *device) { return device == mouse || device == keyboard; }),
	                     m_inputDevices.end());

	// delete remaining input devices (if any)
	for (auto *device : m_inputDevices)
	{
		delete device;
	}
	m_inputDevices.clear();
	m_mice.clear();
	m_keyboards.clear();

	// reset the static unique_ptrs
	s_mouseInstance.reset();
	s_keyboardInstance.reset();

	debugLog("Engine: Freeing timer...\n");
	SAFE_DELETE(m_timer);

	debugLog("Engine: Freeing graphics...\n");
	s_graphicsInstance.reset();

	debugLog("Engine: Freeing math...\n");
	SAFE_DELETE(m_math);

	if (m_bIsRestarting)
	{
		debugLog("Engine: Resetting ConVar callbacks...\n");
		convar->resetAllConVarCallbacks();
		debugLog("Engine: Restarting...\n");
	}
	else
	{
		debugLog("Engine: Goodbye.\n");
	}

	engine = NULL;
}

void Engine::loadApp()
{
	// load core default resources
	debugLog("Engine: Loading default resources ...\n");
	{
		resourceManager->loadFont("weblysleekuisb.ttf", "FONT_DEFAULT", 15, true, env->getDPI());
		resourceManager->loadFont("tahoma.ttf", "FONT_CONSOLE", 8, false, 96);
	}
	debugLog("Engine: Loading default resources done.\n");

	// load other default resources and things which are not strictly necessary
	{
		Image *missingTexture = resourceManager->createImage(512, 512);
		resourceManager->setResourceName(missingTexture, "MISSING_TEXTURE");
		for (int x = 0; x < 512; x++)
		{
			for (int y = 0; y < 512; y++)
			{
				int rowCounter = (x / 64);
				int columnCounter = (y / 64);
				Color color = (((rowCounter + columnCounter) % 2 == 0) ? rgb(255, 0, 221) : rgb(0, 0, 0));
				missingTexture->setPixel(x, y, color);
			}
		}
		missingTexture->load();

		// create engine gui
		m_guiContainer = new CBaseUIContainer(0, 0, static_cast<float>(getScreenWidth()), static_cast<float>(getScreenHeight()), "");
		m_consoleBox = new ConsoleBox();
		m_visualProfiler = new VisualProfiler();
		m_guiContainer->addBaseUIElement(m_visualProfiler);
		m_guiContainer->addBaseUIElement(m_consoleBox);

		// (engine hardcoded hotkeys come first, then engine gui)
		keyboard->addListener(m_guiContainer, true);
		keyboard->addListener(this, true);
	}

	debugLog("\nEngine: Loading app ...\n");
	{
		//*****************//
		//	Load App here  //
		//*****************//

		s_appInstance = std::make_unique<App>();
		app = s_appInstance.get();
		runtime_assert(app, "App failed to initialize!");
		// start listening to the default keyboard input
		keyboard->addListener(app);
	}
	debugLog("Engine: Loading app done.\n\n");
}

void Engine::onPaint()
{
	VPROF_BUDGET("Engine::onPaint", VPROF_BUDGETGROUP_DRAW);
	if (m_bBlackout || m_bIsMinimized)
		return;

	m_bDrawing = true;
	{
		// begin
		{
			VPROF_BUDGET("Graphics::beginScene", VPROF_BUDGETGROUP_DRAW);
			graphics->beginScene();
		}

		// middle
		{
			if (app != NULL)
			{
				VPROF_BUDGET("App::draw", VPROF_BUDGETGROUP_DRAW);
				app->draw(graphics);
			}

			if (m_guiContainer != NULL)
				m_guiContainer->draw(graphics);

			// debug input devices
			for (auto &m_inputDevice : m_inputDevices)
			{
				m_inputDevice->draw(graphics);
			}

			if (epilepsy.getBool())
			{
				graphics->setColor(rgb(rand() % 256, rand() % 256, rand() % 256));
				graphics->fillRect(0, 0, engine->getScreenWidth(), engine->getScreenHeight());
			}
		}

		// end
		{
			VPROF_BUDGET("Graphics::endScene", VPROF_BUDGETGROUP_DRAW_SWAPBUFFERS);
			graphics->endScene();
		}
	}
	m_bDrawing = false;

	m_iFrameCount++;
}

void Engine::onUpdate()
{
	VPROF_BUDGET("Engine::onUpdate", VPROF_BUDGETGROUP_UPDATE);

	if (m_bBlackout || (m_bIsMinimized && !(networkHandler->isClient() || networkHandler->isServer())))
		return;

	// update time
	{
		m_timer->update();
		m_dRunTime = m_timer->getElapsedTime();
		m_dFrameTime *= (double)host_timescale.getFloat();
		m_dTime += m_dFrameTime;
		if (engine_throttle.getBool())
		{
			// it's more like a crude estimate but it gets the job done for use as a throttle
			if ((m_fVsyncFrameCounterTime += static_cast<float>(m_dFrameTime)) > env->getDisplayRefreshTime())
			{
				m_fVsyncFrameCounterTime = 0.0f;
				++m_iVsyncFrameCount;
			}
		}
	}

	// handle pending queued resolution changes
	if (m_bResolutionChange)
	{
		m_bResolutionChange = false;

		if (debug_engine.getBool())
			debugLog("Engine: executing pending queued resolution change to ({}, {})\n", (int)m_vNewScreenSize.x, (int)m_vNewScreenSize.y);

		onResolutionChange(m_vNewScreenSize);
	}

	// update miscellaneous engine subsystems
	{
		for (auto &m_inputDevice : m_inputDevices)
		{
			m_inputDevice->update();
		}

		{
			VPROF_BUDGET("AnimationHandler::update", VPROF_BUDGETGROUP_UPDATE);
			animationHandler->update();
		}

		{
			VPROF_BUDGET("SoundEngine::update", VPROF_BUDGETGROUP_UPDATE);
			soundEngine->update();
		}

		{
			VPROF_BUDGET("ResourceManager::update", VPROF_BUDGETGROUP_UPDATE);
			resourceManager->update();
		}

		// update gui
		if (m_guiContainer != NULL)
			m_guiContainer->update();

		// execute queued commands
		// TODO: this is shit
		if (Console::g_commandQueue.size() > 0)
		{
			for (const auto &i : Console::g_commandQueue)
			{
				Console::processCommand(i);
			}
			Console::g_commandQueue = std::vector<UString>(); // reset
		}

		// update networking
		{
			VPROF_BUDGET("NetworkHandler::update", VPROF_BUDGETGROUP_UPDATE);
			networkHandler->update();
		}
	}

	// update app
	if (app != NULL)
	{
		VPROF_BUDGET("App::update", VPROF_BUDGETGROUP_UPDATE);
		app->update();
	}

	// update environment (after app, at the end here)
	{
		VPROF_BUDGET("Environment::update", VPROF_BUDGETGROUP_UPDATE);
		env->update();
	}
}

void Engine::onFocusGained()
{
	m_bHasFocus = true;

	if (debug_engine.getBool())
		debugLog("Engine: got focus\n");

	if (app != NULL)
		app->onFocusGained();
}

void Engine::onFocusLost()
{
	m_bHasFocus = false;

	if (debug_engine.getBool())
		debugLog("Engine: lost focus\n");

	for (auto &m_keyboard : m_keyboards)
	{
		m_keyboard->reset();
	}

	if (app != NULL)
		app->onFocusLost();

	// auto minimize on certain conditions
	if (env->isFullscreen() || env->isFullscreenWindowedBorderless())
	{
		if ((!env->isFullscreenWindowedBorderless() && minimize_on_focus_lost_if_fullscreen.getBool()) ||
		    (env->isFullscreenWindowedBorderless() && minimize_on_focus_lost_if_borderless_windowed_fullscreen.getBool()))
		{
			env->minimize();
		}
	}
}

void Engine::onMinimized()
{
	m_bIsMinimized = true;
	m_bHasFocus = false;

	if (debug_engine.getBool())
		debugLog("Engine: window minimized\n");

	if (app != NULL)
		app->onMinimized();
}

void Engine::onMaximized()
{
	m_bIsMinimized = false;

	if (debug_engine.getBool())
		debugLog("Engine: window maximized\n");
}

void Engine::onRestored()
{
	m_bIsMinimized = false;

	if (debug_engine.getBool())
		debugLog("Engine: window restored\n");

	if (app != NULL)
		app->onRestored();
}

void Engine::onResolutionChange(Vector2 newResolution)
{
	debugLog(
	    0xff00ff00, "Engine: onResolutionChange() ({}, {}) -> ({}, {})\n", (int)m_vScreenSize.x, (int)m_vScreenSize.y, (int)newResolution.x, (int)newResolution.y);

	// NOTE: Windows [Show Desktop] button in the superbar causes (0,0)
	if (newResolution.x < 2 || newResolution.y < 2)
	{
		m_bIsMinimized = true;
		newResolution = Vector2(2, 2);
	}

	// to avoid double resolutionChange
	m_bResolutionChange = false;
	m_vNewScreenSize = newResolution;

	if (m_guiContainer != NULL)
		m_guiContainer->setSize(newResolution.x, newResolution.y);
	if (m_consoleBox != NULL)
		m_consoleBox->onResolutionChange(newResolution);

	// update everything
	m_vScreenSize = newResolution;
	m_screenRect = {
	    {0, 0},
        newResolution
    };

	if (graphics != NULL)
		graphics->onResolutionChange(newResolution);
	if (app != NULL)
		app->onResolutionChanged(newResolution);
}

void Engine::onDPIChange()
{
	debugLog(0xff00ff00, "Engine: DPI changed to {}\n", env->getDPI());

	if (app != NULL)
		app->onDPIChanged();
}

void Engine::onShutdown()
{
	env->setCursorClip(false, {});
	if (m_bBlackout || (app != NULL && !app->onShutdown()))
		return;

	m_bBlackout = true;
}

// hardcoded engine hotkeys
void Engine::onKeyDown(KeyboardEvent &e)
{
	auto keyCode = e.getKeyCode();
	// handle ALT+F4 quit
	if (keyboard->isAltDown() && keyCode == KEY_F4)
	{
		shutdown();
		e.consume();
		return;
	}

	// handle ALT+ENTER fullscreen toggle
	if (keyboard->isAltDown() && keyCode == KEY_ENTER)
	{
		toggleFullscreen();
		e.consume();
		return;
	}

	// handle CTRL+F11 profiler toggle
	if (keyboard->isControlDown() && keyCode == KEY_F11)
	{
		ConVar *vprof = convar->getConVarByName("vprof");
		vprof->setValue(vprof->getBool() ? 0.0f : 1.0f);
		e.consume();
		return;
	}

	// handle profiler display mode change
	if (keyboard->isControlDown() && keyCode == KEY_TAB)
	{
		const ConVar *vprof = convar->getConVarByName("vprof");
		if (vprof->getBool())
		{
			if (keyboard->isShiftDown())
				m_visualProfiler->decrementInfoBladeDisplayMode();
			else
				m_visualProfiler->incrementInfoBladeDisplayMode();
			e.consume();
			return;
		}
	}
}

void Engine::shutdown()
{
	onShutdown();
	env->shutdown();
}

void Engine::restart()
{
	onShutdown();
	// m_bIsRestarting = true; // TODO
	env->restart();
}

void Engine::focus()
{
	env->focus();
}

void Engine::center()
{
	env->center();
}

void Engine::toggleFullscreen()
{
	if (env->isFullscreen())
		env->disableFullscreen();
	else
		env->enableFullscreen();
}

void Engine::disableFullscreen()
{
	env->disableFullscreen();
}

void Engine::showMessageInfo(UString title, UString message)
{
	debugLog("INFO: [{:s}] | {:s}\n", title.toUtf8(), message.toUtf8());
	env->showMessageInfo(title, message);
}

void Engine::showMessageWarning(UString title, UString message)
{
	debugLog("WARNING: [{:s}] | {:s}\n", title.toUtf8(), message.toUtf8());
	env->showMessageWarning(title, message);
}

void Engine::showMessageError(UString title, UString message)
{
	debugLog("ERROR: [{:s}] | {:s}\n", title.toUtf8(), message.toUtf8());
	env->showMessageError(title, message);
}

void Engine::showMessageErrorFatal(UString title, UString message)
{
	debugLog("FATAL ERROR: [{:s}] | {:s}\n", title.toUtf8(), message.toUtf8());
	env->showMessageErrorFatal(title, message);
}

void Engine::requestResolutionChange(Vector2 newResolution)
{
	// FIXME: sdl2->sdl3 broke this check on non-fullscreen startup
	// if (newResolution == m_vNewScreenSize) return;
	// debugLog("newRes ({},{}) m_vNewScreenSize ({},{})\n", (int)newResolution.x, (int)newResolution.y, (int)m_vNewScreenSize.x, (int)m_vNewScreenSize.y);
	m_vNewScreenSize = newResolution;
	m_bResolutionChange = true;
}

void Engine::setFrameTime(double delta)
{
	// NOTE: clamp to between 10000 fps and 1 fps, very small/big timesteps could cause problems
	m_dFrameTime = std::clamp<double>(delta, 0.0001, 1.0);
}

void Engine::logToConsole(std::optional<Color> color, UString msg)
{
	if (m_consoleBox != nullptr)
	{
		if (color.has_value())
			m_consoleBox->log(msg, color.value());
		else
			m_consoleBox->log(msg);
	}

	if (m_console != nullptr)
	{
		if (color.has_value())
			m_console->log(msg, color.value());
		else
			m_console->log(msg);
	}
}

//**********************//
//	Engine ConCommands	//
//**********************//

void Engine::onEngineThrottleChanged(float newVal)
{
	const bool enable = !!static_cast<int>(newVal);
	if (!enable)
	{
		m_fVsyncFrameCounterTime = 0.0f;
		m_iVsyncFrameCount = 0;
	}
}

void _exit(void)
{
	engine->shutdown();
}

void _restart(void)
{
	engine->restart();
}

void _printsize(void)
{
	Vector2 s = engine->getScreenSize();
	debugLog("Engine: screenSize = ({:f}, {:f})\n", s.x, s.y);
}

void _fullscreen(void)
{
	engine->toggleFullscreen();
}

void _borderless(void)
{
	ConVar *fullscreen_windowed_borderless_ref = convar->getConVarByName("fullscreen_windowed_borderless");
	if (fullscreen_windowed_borderless_ref != NULL)
	{
		if (fullscreen_windowed_borderless_ref->getBool())
		{
			fullscreen_windowed_borderless_ref->setValue(0.0f);
			if (env->isFullscreen())
				env->disableFullscreen();
		}
		else
		{
			fullscreen_windowed_borderless_ref->setValue(1.0f);
			if (!env->isFullscreen())
				env->enableFullscreen();
		}
	}
}

void _windowed(UString args)
{
	env->disableFullscreen();

	if (args.length() < 7)
		return;

	std::vector<UString> resolution = args.split("x");
	if (resolution.size() != 2)
		debugLog("Error: Invalid parameter count for command 'windowed'! (Usage: e.g. \"windowed 1280x720\")");
	else
	{
		int width = resolution[0].toFloat();
		int height = resolution[1].toFloat();

		if (width < 300 || height < 240)
			debugLog("Error: Invalid values for resolution for command 'windowed'!");
		else
		{
			env->setWindowSize(width, height);
			env->center();
		}
	}
}

void _minimize(void)
{
	env->minimize();
}

void _maximize(void)
{
	env->maximize();
}

void _toggleresizable(void)
{
	env->setWindowResizable(!env->isWindowResizable());
}

void _focus(void)
{
	engine->focus();
}

void _center(void)
{
	engine->center();
}

void _version(void)
{
	debugLog("McEngine v4 - Build Date: {:s}, {:s}\n", __DATE__, __TIME__);
}

void _errortest(void)
{
	engine->showMessageError("Error Test", "This is an error message, fullscreen mode should be disabled and you should be able to read this");
}

void _crash(void)
{
	__builtin_trap();
}

void _dpiinfo(void)
{
	debugLog("env->getDPI() = {}, env->getDPIScale() = {:f}\n", env->getDPI(), env->getDPIScale());
}

ConVar _exit_("exit", FCVAR_NONE, _exit);
ConVar _shutdown_("shutdown", FCVAR_NONE, _exit);
ConVar _restart_("restart", FCVAR_NONE, _restart);
ConVar _printsize_("printsize", FCVAR_NONE, _printsize);
ConVar _fullscreen_("fullscreen", FCVAR_NONE, _fullscreen);
ConVar _borderless_("borderless", FCVAR_NONE, _borderless);
ConVar _windowed_("windowed", FCVAR_NONE, _windowed);
ConVar _minimize_("minimize", FCVAR_NONE, _minimize);
ConVar _maximize_("maximize", FCVAR_NONE, _maximize);
ConVar _resizable_toggle_("resizable_toggle", FCVAR_NONE, _toggleresizable);
ConVar _focus_("focus", FCVAR_NONE, _focus);
ConVar _center_("center", FCVAR_NONE, _center);
ConVar _version_("version", FCVAR_NONE, _version);
ConVar _errortest_("errortest", FCVAR_NONE, _errortest);
ConVar _crash_("crash", FCVAR_NONE, _crash);
ConVar _dpiinfo_("dpiinfo", FCVAR_NONE, _dpiinfo);
