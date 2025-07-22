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

//******************//
//	Engine Globals	//
//******************//

std::unique_ptr<Mouse> mouse = nullptr;
std::unique_ptr<Keyboard> keyboard = nullptr;
std::unique_ptr<App> app = nullptr;
std::unique_ptr<Graphics> g = nullptr;
std::unique_ptr<SoundEngine> soundEngine = nullptr;
std::unique_ptr<ResourceManager> resourceManager = nullptr;
std::unique_ptr<NetworkHandler> networkHandler = nullptr;
std::unique_ptr<AnimationHandler> animationHandler = nullptr;
std::unique_ptr<SteamworksInterface> steam = nullptr;
std::unique_ptr<DiscordInterface> discord = nullptr;

Engine *engine = NULL;

Console *Engine::m_console = NULL;
ConsoleBox *Engine::m_consoleBox = NULL;

//**************//
//	End Globals	//
//**************//

//**********************//
//	Engine ConCommands	//
//**********************//

namespace {
void _borderless(void)
{
	if (cv::fullscreen_windowed_borderless.getBool())
	{
		cv::fullscreen_windowed_borderless.setValue(0.0f);
		if (env->isFullscreen())
			env->disableFullscreen();
	}
	else
	{
		cv::fullscreen_windowed_borderless.setValue(1.0f);
		if (!env->isFullscreen())
			env->enableFullscreen();
	}
}

void _windowed(const UString &args)
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
}

void Engine::printVersion()
{
	Engine::logRaw("[Engine] McEngine v5 - Build Date: {:s}, {:s}\n", __DATE__, __TIME__);
}

void Engine::onEngineThrottleChanged(float newVal)
{
	const bool enable = !!static_cast<int>(newVal);
	if (!enable)
	{
		m_fVsyncFrameCounterTime = 0.0f;
		m_iVsyncFrameCount = 0;
	}
}

namespace cv {
ConVar host_timescale("host_timescale", 1.0f, FCVAR_CHEAT, "Scale by which the engine measures elapsed time, affects engine->getTime()",
	[](float value) -> void {
		if (value < 0.01f)
		{
			Engine::logRaw("[Engine] host_timescale value must be >= 0.01!\n");
			cv::host_timescale.setValue(1.0f, false);
		}
});

ConVar engine_throttle("engine_throttle", true, FCVAR_NONE, "limit some engine component updates to improve performance (non-gameplay-related, only turn this off if you like lower performance for no reason)");

ConVar epilepsy("epilepsy", false, FCVAR_NONE);
ConVar debug_engine("debug_engine", false, FCVAR_NONE);
ConVar minimize_on_focus_lost_if_fullscreen("minimize_on_focus_lost_if_fullscreen", true, FCVAR_NONE);
ConVar minimize_on_focus_lost_if_borderless_windowed_fullscreen("minimize_on_focus_lost_if_borderless_windowed_fullscreen", false, FCVAR_NONE);
ConVar disable_windows_key("disable_windows_key", false, FCVAR_NONE, "set to 0/1 to disable/enable the Windows/Super key");

ConVar exit("exit", FCVAR_NONE, []() -> void {engine ? engine->shutdown() : (void)0;});
ConVar shutdown("shutdown", FCVAR_NONE, []() -> void {engine ? engine->shutdown() : (void)0;});
ConVar restart("restart", FCVAR_NONE, []() -> void {engine ? engine->restart() : (void)0;});
ConVar printsize("printsize", FCVAR_NONE, []() -> void {if (engine) {Vector2 s = engine->getScreenSize(); Engine::logRaw("[Engine] screenSize = ({:f}, {:f})\n", s.x, s.y);}});
ConVar fullscreen("fullscreen", FCVAR_NONE, []() -> void {engine ? engine->toggleFullscreen() : (void)0;});
ConVar borderless("borderless", FCVAR_NONE, []() -> void {_borderless();});
ConVar windowed("windowed", FCVAR_NONE, CFUNC(_windowed));
ConVar minimize("minimize", FCVAR_NONE, []() -> void {env ? env->minimize() : (void)0;});
ConVar maximize("maximize", FCVAR_NONE, []() -> void {env ? env->maximize() : (void)0;});
ConVar resizable_toggle("resizable_toggle", FCVAR_NONE, []() -> void {env ? env->setWindowResizable(!env->isWindowResizable()) : (void)0;});
ConVar focus("focus", FCVAR_NONE, []() -> void {engine ? engine->focus() : (void)0;});
ConVar center("center", FCVAR_NONE, []() -> void {engine ? engine->center() : (void)0;});
ConVar version("version", FCVAR_NONE, CFUNC(Engine::printVersion));
ConVar errortest("errortest", FCVAR_NONE, []() -> void {engine ? engine->showMessageError("Error Test", "This is an error message, fullscreen mode should be disabled and you should be able to read this") : (void)0;});
ConVar crash("crash", FCVAR_NONE, []() -> void {std::abort();});
ConVar dpiinfo("dpiinfo", FCVAR_NONE, []() -> void {env ? Engine::logRaw("[Engine] DPI: {}, DPIScale: {:.4f}\n", env->getDPI(), env->getDPIScale()) : (void)0;});
}
//******************//
//	End ConCommands	//
//******************//

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
	printVersion();
	debugLog("cmdline: {:s}\n", UString::join(env->getCommandLine()));

	// timing
	m_timer = new Timer(false);
	m_dTime = 0;
	m_dRunTime = 0;
	m_iFrameCount = 0;
	m_iVsyncFrameCount = 0;
	m_fVsyncFrameCounterTime = 0.0f;
	m_dFrameTime = 0.016;

	cv::engine_throttle.setCallback(SA::MakeDelegate<&Engine::onEngineThrottleChanged>(this));

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
		mouse = std::make_unique<Mouse>();
		runtime_assert(mouse.get(), "Mouse failed to initialize!");
		m_inputDevices.push_back(mouse.get());
		m_mice.push_back(mouse.get());

		keyboard = std::make_unique<Keyboard>();
		runtime_assert(keyboard.get(), "Keyboard failed to initialize!");
		m_inputDevices.push_back(keyboard.get());
		m_keyboards.push_back(keyboard.get());

		// create graphics through environment
		g.reset(env->createRenderer());
		runtime_assert(g.get(), "Graphics failed to initialize!");
		g->init(); // needs init() separation due to potential graphics access

		// make unique_ptrs for the rest
		resourceManager = std::make_unique<ResourceManager>();
		runtime_assert(resourceManager.get(), "Resource manager menu failed to initialize!");

		SoundEngine::SndEngineType type = Env::cfg(AUD::SOLOUD) ? SoundEngine::SOLOUD : Env::cfg(AUD::BASS) ? SoundEngine::BASS : Env::cfg(AUD::SDL) ? SoundEngine::SDL : SoundEngine::BASS;
		{
			auto args = env->getLaunchArgs();
			auto soundString = args["-sound"].value_or("soloud").trim();
			soundString.lowerCase();
			if (Env::cfg(AUD::SOLOUD) && soundString == "soloud")
				type = SoundEngine::SOLOUD;
			else if (Env::cfg(AUD::BASS) && soundString == "bass")
				type = SoundEngine::BASS;
			else if (Env::cfg(AUD::SDL) && soundString == "sdl")
				type = SoundEngine::SDL;
		}
		soundEngine.reset(SoundEngine::createSoundEngine(type));
		runtime_assert(soundEngine.get(), "Sound engine failed to initialize!");

		animationHandler = std::make_unique<AnimationHandler>();
		runtime_assert(animationHandler.get(), "Animation handler failed to initialize!");

		networkHandler = std::make_unique<NetworkHandler>();
		runtime_assert(networkHandler.get(), "Network handler failed to initialize!");

		if constexpr (Env::cfg(FEAT::STEAM))
		{
			steam = std::make_unique<SteamworksInterface>();
			runtime_assert(steam.get(), "Steam integration failed to initialize!");
		}

		discord = std::make_unique<DiscordInterface>(); // TODO: allow disabling
		runtime_assert(discord.get(), "Discord integration failed to initialize!");

		// default launch overrides
		g->setVSync(false);

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
	app.reset();

	if (m_console != NULL)
		showMessageErrorFatal("Engine Error", "m_console not set to NULL before shutdown!");

	debugLog("Engine: Freeing engine GUI...\n");
	{
		m_console = NULL;
		m_consoleBox = NULL;
	}
	SAFE_DELETE(m_guiContainer);

	debugLog("Engine: Freeing Discord...\n");
	discord.reset();

	if constexpr (Env::cfg(FEAT::STEAM))
	{
		debugLog("Engine: Freeing Steam...\n");
		steam.reset();
	}

	debugLog("Engine: Freeing network handler...\n");
	networkHandler.reset();

	debugLog("Engine: Freeing animation handler...\n");
	animationHandler.reset();

	debugLog("Engine: Freeing resource manager...\n");
	resourceManager.reset();

	debugLog("Engine: Freeing sound engine...\n");
	soundEngine.reset();

	debugLog("Engine: Freeing graphics...\n");
	g.reset();

	debugLog("Engine: Freeing input devices...\n");
	// first remove the mouse and keyboard from the input devices
	std::erase_if(m_inputDevices, [](InputDevice *device) { return device == mouse.get() || device == keyboard.get(); });

	// delete remaining input devices (if any)
	for (auto *device : m_inputDevices)
	{
		delete device;
	}
	m_inputDevices.clear();
	m_mice.clear();
	m_keyboards.clear();

	// reset the static unique_ptrs
	mouse.reset();
	keyboard.reset();

	debugLog("Engine: Freeing timer...\n");
	SAFE_DELETE(m_timer);

	debugLog("Engine: Freeing fonts...\n");
	McFont::cleanupSharedResources();

	debugLog("Engine: Freeing math...\n");
	SAFE_DELETE(m_math);

	if (m_bIsRestarting)
	{
		debugLog("Engine: Resetting ConVar callbacks...\n");
		ConVar::resetAllConVarCallbacks();
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

		app = std::make_unique<App>();
		runtime_assert(app.get(), "App failed to initialize!");
		// start listening to the default keyboard input
		keyboard->addListener(app.get());
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
			g->beginScene();
		}

		// middle
		{
			if (app != NULL)
			{
				VPROF_BUDGET("App::draw", VPROF_BUDGETGROUP_DRAW);
				app->draw();
			}

			if (m_guiContainer != NULL)
				m_guiContainer->draw();

			// debug input devices
			for (auto &m_inputDevice : m_inputDevices)
			{
				m_inputDevice->draw();
			}

			if (cv::epilepsy.getBool())
			{
				g->setColor(rgb(rand() % 256, rand() % 256, rand() % 256));
				g->fillRect(0, 0, engine->getScreenWidth(), engine->getScreenHeight());
			}
		}

		// end
		{
			VPROF_BUDGET("Graphics::endScene", VPROF_BUDGETGROUP_DRAW_SWAPBUFFERS);
			g->endScene();
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
		m_dFrameTime *= (double)cv::host_timescale.getFloat();
		m_dTime += m_dFrameTime;
		if (cv::engine_throttle.getBool())
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

		if (cv::debug_engine.getBool())
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

	if (cv::debug_engine.getBool())
		debugLog("Engine: got focus\n");

	if (app != NULL)
		app->onFocusGained();
}

void Engine::onFocusLost()
{
	m_bHasFocus = false;

	if (cv::debug_engine.getBool())
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
		if ((!env->isFullscreenWindowedBorderless() && cv::minimize_on_focus_lost_if_fullscreen.getBool()) ||
		    (env->isFullscreenWindowedBorderless() && cv::minimize_on_focus_lost_if_borderless_windowed_fullscreen.getBool()))
		{
			env->minimize();
		}
	}
}

void Engine::onMinimized()
{
	m_bIsMinimized = true;
	m_bHasFocus = false;

	if (cv::debug_engine.getBool())
		debugLog("Engine: window minimized\n");

	if (app != NULL)
		app->onMinimized();
}

void Engine::onMaximized()
{
	m_bIsMinimized = false;

	if (cv::debug_engine.getBool())
		debugLog("Engine: window maximized\n");
}

void Engine::onRestored()
{
	m_bIsMinimized = false;

	if (cv::debug_engine.getBool())
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

	if (g != NULL)
		g->onResolutionChange(newResolution);
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
	if (keyboard->isAltDown() && (keyCode == KEY_ENTER || keyCode == KEY_NUMPAD_ENTER))
	{
		toggleFullscreen();
		e.consume();
		return;
	}

	// handle CTRL+F11 profiler toggle
	if (keyboard->isControlDown() && keyCode == KEY_F11)
	{
		cv::vprof.setValue(cv::vprof.getBool() ? 0.0f : 1.0f);
		e.consume();
		return;
	}

	// handle profiler display mode change
	if (keyboard->isControlDown() && keyCode == KEY_TAB)
	{
		if (cv::vprof.getBool())
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

void Engine::showMessageInfo(const UString& title, const UString& message)
{
	debugLog("INFO: [{:s}] | {:s}\n", title.toUtf8(), message.toUtf8());
	env->showMessageInfo(title, message);
}

void Engine::showMessageWarning(const UString& title, const UString& message)
{
	debugLog("WARNING: [{:s}] | {:s}\n", title.toUtf8(), message.toUtf8());
	env->showMessageWarning(title, message);
}

void Engine::showMessageError(const UString& title, const UString& message)
{
	debugLog("ERROR: [{:s}] | {:s}\n", title.toUtf8(), message.toUtf8());
	env->showMessageError(title, message);
}

void Engine::showMessageErrorFatal(const UString& title, const UString& message)
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

void Engine::logToConsole(std::optional<Color> color, const UString &msg)
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
