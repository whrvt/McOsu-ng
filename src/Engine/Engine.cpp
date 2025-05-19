//================ Copyright (c) 2012, PG, All rights reserved. =================//
//
// Purpose:		core
//
// $NoKeywords: $engine
//===============================================================================//

#include <cstdio>

#ifdef MCENGINE_FEATURE_MULTITHREADING
#include <mutex>
#endif

#include "AnimationHandler.h"
#include "ConVar.h"
#include "ContextMenu.h"
#include "DiscordInterface.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "NetworkHandler.h"
#include "OpenVRInterface.h"
#include "Profiler.h"
#include "ResourceManager.h"
#include "SoundEngine.h"
#include "SteamworksInterface.h"
#include "Timing.h"
#include "VulkanInterface.h"

#include "CBaseUIContainer.h"

#include "Console.h"
#include "ConsoleBox.h"
#include "VisualProfiler.h"

#include "Engine.h"

#include <utility>

//********************//
//	Include App here  //
//********************//

#include "Osu.h"

void _version(void);
void _host_timescale_(UString oldValue, UString newValue);
ConVar host_timescale("host_timescale", 1.0f, FCVAR_CHEAT, "Scale by which the engine measures elapsed time, affects engine->getTime()", _host_timescale_);
void _host_timescale_(UString oldValue, UString newValue)
{
	if (newValue.toFloat() < 0.01f)
	{
		debugLog(0xffff4444, UString::format("Value must be >= 0.01!\n").toUtf8());
		host_timescale.setValue(1.0f);
	}
}
ConVar epilepsy("epilepsy", false, FCVAR_NONE);
ConVar debug_engine("debug_engine", false, FCVAR_NONE);
ConVar minimize_on_focus_lost_if_fullscreen("minimize_on_focus_lost_if_fullscreen", true, FCVAR_NONE);
ConVar minimize_on_focus_lost_if_borderless_windowed_fullscreen("minimize_on_focus_lost_if_borderless_windowed_fullscreen", false, FCVAR_NONE);
ConVar _disable_windows_key("disable_windows_key", false, FCVAR_NONE, "set to 0/1 to disable/enable the Windows/Super key");

std::unique_ptr<Mouse> Engine::s_mouseInstance = nullptr;
std::unique_ptr<Keyboard> Engine::s_keyboardInstance = nullptr;
std::unique_ptr<App> Engine::s_appInstance = nullptr;
std::unique_ptr<Graphics> Engine::s_graphicsInstance = nullptr;
std::unique_ptr<SoundEngine> Engine::s_soundEngineInstance = nullptr;
std::unique_ptr<ResourceManager> Engine::s_resourceManagerInstance = nullptr;
std::unique_ptr<NetworkHandler> Engine::s_networkHandlerInstance = nullptr;
std::unique_ptr<OpenVRInterface> Engine::s_openVRInstance = nullptr;
std::unique_ptr<VulkanInterface> Engine::s_vulkanInstance = nullptr;
std::unique_ptr<ContextMenu> Engine::s_contextMenuInstance = nullptr;
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
OpenVRInterface *openVR = nullptr;
VulkanInterface *vulkan = nullptr;
ContextMenu *contextMenu = nullptr;
AnimationHandler *animationHandler = nullptr;
SteamworksInterface *steam = nullptr;
DiscordInterface *discord = nullptr;

Engine *engine = NULL;

Console *Engine::m_console = NULL;
ConsoleBox *Engine::m_consoleBox = NULL;

Engine::Engine(const char *args)
{
	engine = this;
	m_sArgs = UString(args);

	m_guiContainer = nullptr;
	m_visualProfiler = nullptr;

	// disable output buffering (else we get multithreading issues due to blocking)
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	// print debug information
	debugLog("-= Engine Startup =-\n");
	_version();
	debugLog("Engine: args = %s\n", m_sArgs.toUtf8());

	// timing
	m_timer = new Timer(false);
	m_dTime = 0;
	m_dRunTime = 0;
	m_iFrameCount = 0;
	m_dFrameTime = 0.016f;

	// window
	m_bBlackout = false;
	m_bHasFocus = false;
	m_bIsMinimized = false;

	// screen
	m_bResolutionChange = false;
	m_vScreenSize = env->getWindowSize();
	m_vNewScreenSize = m_vScreenSize;

	debugLog("Engine: ScreenSize = (%ix%i)\n", (int)m_vScreenSize.x, (int)m_vScreenSize.y);

	// custom
	m_bDrawing = false;

	// math (its own singleton)
	m_math = new McMath();

	// initialize all engine subsystems (the order does matter!)
	debugLog("\nEngine: Initializing subsystems ...\n");
	{
		// input devices
		s_mouseInstance = std::make_unique<Mouse>();
		mouse = s_mouseInstance.get();
		m_inputDevices.push_back(mouse);
		m_mice.push_back(mouse);

		s_keyboardInstance = std::make_unique<Keyboard>();
		keyboard = s_keyboardInstance.get();
		m_inputDevices.push_back(keyboard);
		m_keyboards.push_back(keyboard);

		// init platform specific interfaces
		s_vulkanInstance = std::make_unique<VulkanInterface>();
		vulkan = s_vulkanInstance.get(); // needs to be created before Graphics

		// create graphics through environment
		graphics = env->createRenderer();
		{
			graphics->init(); // needs init() separation due to potential graphics access
		}
		s_graphicsInstance.reset(graphics);

		contextMenu = env->createContextMenu();
		s_contextMenuInstance.reset(contextMenu);

		// make unique_ptrs for the rest
		s_resourceManagerInstance = std::make_unique<ResourceManager>();
		resourceManager = s_resourceManagerInstance.get();

		s_soundEngineInstance.reset(SoundEngine::createSoundEngine());
		soundEngine = s_soundEngineInstance.get();

		s_animationHandlerInstance = std::make_unique<AnimationHandler>();
		animationHandler = s_animationHandlerInstance.get();

		s_openVRInstance = std::make_unique<OpenVRInterface>();
		openVR = s_openVRInstance.get();

		s_networkHandlerInstance = std::make_unique<NetworkHandler>();
		networkHandler = s_networkHandlerInstance.get();

		if constexpr (Env::cfg(FEAT::STEAM))
		{
			s_steamInstance = std::make_unique<SteamworksInterface>();
			steam = s_steamInstance.get();
		}

		s_discordInstance = std::make_unique<DiscordInterface>();
		discord = s_discordInstance.get();

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

	debugLog("Engine: Freeing OpenVR...\n");
	s_openVRInstance.reset();

	debugLog("Engine: Freeing Sound...\n");
	s_soundEngineInstance.reset();

	debugLog("Engine: Freeing context menu...\n");
	s_contextMenuInstance.reset();

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

	debugLog("Engine: Freeing Vulkan...\n");
	s_vulkanInstance.reset();

	debugLog("Engine: Freeing math...\n");
	SAFE_DELETE(m_math);

	debugLog("Engine: Goodbye.");

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
		missingTexture->setName("MISSING_TEXTURE");
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

		// (engine gui comes first)
		keyboard->addListener(m_guiContainer, true);
	}

	debugLog("\nEngine: Loading app ...\n");
	{
		//*****************//
		//	Load App here  //
		//*****************//

		s_appInstance = std::make_unique<Osu>();
		app = s_appInstance.get();

		// start listening to the default keyboard input
		if (app != nullptr)
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
			for (size_t i = 0; i < m_inputDevices.size(); i++)
			{
				m_inputDevices[i]->draw(graphics);
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
	}

	// handle pending queued resolution changes
	if (m_bResolutionChange)
	{
		m_bResolutionChange = false;

		if (debug_engine.getBool())
			debugLog("Engine: executing pending queued resolution change to (%i, %i)\n", (int)m_vNewScreenSize.x, (int)m_vNewScreenSize.y);

		onResolutionChange(m_vNewScreenSize);
	}

	// update miscellaneous engine subsystems
	{
		for (auto &m_inputDevice : m_inputDevices)
		{
			m_inputDevice->update();
		}

		openVR->update(); // (this also handles its input devices)

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

	for (size_t i = 0; i < m_keyboards.size(); i++)
	{
		m_keyboards[i]->reset();
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
	debugLog(0xff00ff00, "Engine: onResolutionChange() (%i, %i) -> (%i, %i)\n", (int)m_vScreenSize.x, (int)m_vScreenSize.y, (int)newResolution.x, (int)newResolution.y);

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
	if (graphics != NULL)
		graphics->onResolutionChange(newResolution);
	if (openVR != NULL)
		openVR->onResolutionChange(newResolution);
	if (app != NULL)
		app->onResolutionChanged(newResolution);
}

void Engine::onDPIChange()
{
	debugLog(0xff00ff00, "Engine: DPI changed to %i\n", env->getDPI());

	if (app != NULL)
		app->onDPIChanged();
}

void Engine::onShutdown()
{
	env->setCursorClip(false, {});
	if (m_bBlackout || (app != NULL && !app->onShutdown()))
		return;

	m_bBlackout = true;
	env->shutdown();
}

void Engine::onMouseMotion(float x, float y, float xRel, float yRel, bool isRawInput)
{
	mouse->onMotion(x, y, xRel, yRel, isRawInput);
}

void Engine::onMouseWheelVertical(int delta)
{
	mouse->onWheelVertical(delta);
}

void Engine::onMouseWheelHorizontal(int delta)
{
	mouse->onWheelHorizontal(delta);
}

void Engine::onMouseButtonChange(int button, bool down)
{
	mouse->onButtonChange(button, down);
}

void Engine::onMouseLeftChange(bool mouseLeftDown)
{
	mouse->onLeftChange(mouseLeftDown);
}

void Engine::onMouseMiddleChange(bool mouseMiddleDown)
{
	mouse->onMiddleChange(mouseMiddleDown);
}

void Engine::onMouseRightChange(bool mouseRightDown)
{
	mouse->onRightChange(mouseRightDown);
}

void Engine::onMouseButton4Change(bool mouse4down)
{
	mouse->onButton4Change(mouse4down);
}

void Engine::onMouseButton5Change(bool mouse5down)
{
	mouse->onButton5Change(mouse5down);
}

void Engine::onKeyboardKeyDown(KEYCODE keyCode)
{
	// hardcoded engine hotkeys
	{
		// handle ALT+F4 quit
		if (keyboard->isAltDown() && keyCode == KEY_F4)
		{
			shutdown();
			return;
		}

		// handle ALT+ENTER fullscreen toggle
		if (keyboard->isAltDown() && keyCode == KEY_ENTER)
		{
			toggleFullscreen();
			return;
		}

		// handle CTRL+F11 profiler toggle
		if (keyboard->isControlDown() && keyCode == KEY_F11)
		{
			ConVar *vprof = convar->getConVarByName("vprof");
			vprof->setValue(vprof->getBool() ? 0.0f : 1.0f);
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
				return;
			}
		}
	}

	keyboard->onKeyDown(keyCode);
}

void Engine::onKeyboardKeyUp(KEYCODE keyCode)
{
	keyboard->onKeyUp(keyCode);
}

void Engine::onKeyboardChar(KEYCODE charCode)
{
	keyboard->onChar(charCode);
}

void Engine::shutdown()
{
	onShutdown();
}

void Engine::restart()
{
	onShutdown();
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
	debugLog("INFO: [%s] | %s\n", title.toUtf8(), message.toUtf8());
	env->showMessageInfo(title, message);
}

void Engine::showMessageWarning(UString title, UString message)
{
	debugLog("WARNING: [%s] | %s\n", title.toUtf8(), message.toUtf8());
	env->showMessageWarning(title, message);
}

void Engine::showMessageError(UString title, UString message)
{
	debugLog("ERROR: [%s] | %s\n", title.toUtf8(), message.toUtf8());
	env->showMessageError(title, message);
}

void Engine::showMessageErrorFatal(UString title, UString message)
{
	debugLog("FATAL ERROR: [%s] | %s\n", title.toUtf8(), message.toUtf8());
	env->showMessageErrorFatal(title, message);
}

void Engine::requestResolutionChange(Vector2 newResolution)
{
	// FIXME: sdl2->sdl3 broke this check on non-fullscreen startup
	// if (newResolution == m_vNewScreenSize) return;
	// debugLog("newRes (%i,%i) m_vNewScreenSize (%i,%i)\n", (int)newResolution.x, (int)newResolution.y, (int)m_vNewScreenSize.x, (int)m_vNewScreenSize.y);
	m_vNewScreenSize = newResolution;
	m_bResolutionChange = true;
}

void Engine::setFrameTime(double delta)
{
	// NOTE: clamp to between 10000 fps and 1 fps, very small/big timesteps could cause problems
	m_dFrameTime = std::clamp<double>(delta, 0.0001, 1.0);
}

void Engine::debugLog_(const char *fmt, va_list args)
{
	if (fmt == NULL)
		return;

	va_list ap2;
	va_copy(ap2, args);

	// write to console
	int numChars = vprintf(fmt, args);

	if (numChars < 1 || numChars > 65534)
		goto cleanup;

	// write to engine console
	{
		char *buffer = new char[numChars + 1];     // +1 for null termination later
		vsnprintf(buffer, numChars + 1, fmt, ap2); // "The generated string has a length of at most n-1, leaving space for the additional terminating null character."
		buffer[numChars] = '\0';                   // null terminate

		UString actualBuffer = UString(buffer);
		delete[] buffer;

		// WARNING: these calls here are not threadsafe by default
		if (m_consoleBox != NULL)
			m_consoleBox->log(actualBuffer);
		if (m_console != NULL)
			m_console->log(actualBuffer);
	}

cleanup:
	va_end(ap2);
}

void Engine::debugLog_(Color color, const char *fmt, va_list args)
{
	if (fmt == NULL)
		return;

	va_list ap2;
	va_copy(ap2, args);

	// write to console
	int numChars = vprintf(fmt, args);

	if (numChars < 1 || numChars > 65534)
		goto cleanup;

	// write to engine console
	{
		char *buffer = new char[numChars + 1];     // +1 for null termination later
		vsnprintf(buffer, numChars + 1, fmt, ap2); // "The generated string has a length of at most n-1, leaving space for the additional terminating null character."
		buffer[numChars] = '\0';                   // null terminate

		UString actualBuffer = UString(buffer);
		delete[] buffer;

		// WARNING: these calls here are not threadsafe by default
		if (m_consoleBox != NULL)
			m_consoleBox->log(actualBuffer, color);
		if (m_console != NULL)
			m_console->log(actualBuffer, color);
	}

cleanup:
	va_end(ap2);
}

void Engine::debugLog_(const char *fmt, ...)
{
	if (fmt == NULL)
		return;

	va_list ap;
	va_start(ap, fmt);

	Engine::debugLog_(fmt, ap);

	va_end(ap);
}

void Engine::debugLog_(Color color, const char *fmt, ...)
{
	if (fmt == NULL)
		return;

	va_list ap;
	va_start(ap, fmt);

	Engine::debugLog_(color, fmt, ap);

	va_end(ap);
}

//**********************//
//	Engine ConCommands	//
//**********************//

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
	debugLog("Engine: screenSize = (%f, %f)\n", s.x, s.y);
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
	debugLog("McEngine v4 - Build Date: %s, %s\n", __DATE__, __TIME__);
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
	debugLog("env->getDPI() = %i, env->getDPIScale() = %f\n", env->getDPI(), env->getDPIScale());
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
