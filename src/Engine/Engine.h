//================ Copyright (c) 2012, PG, All rights reserved. =================//
//
// Purpose:		core
//
// $NoKeywords: $engine
//===============================================================================//

#pragma once
#ifndef ENGINE_H
#define ENGINE_H

#include "App.h"
#include "KeyboardListener.h"
#include "Timing.h"
#include "cbase.h"

#include "fmt/color.h"

#include <source_location>

class Mouse;
class ConVar;
class Keyboard;
class InputDevice;
class SoundEngine;
class NetworkHandler;
class ResourceManager;
class AnimationHandler;
class SteamworksInterface;
class DiscordInterface;

class CBaseUIContainer;
class VisualProfiler;
class ConsoleBox;
class Console;
class McMath;

#ifdef _DEBUG
#define debugLog(...) Engine::ContextLogger::log(std::source_location::current(), __FUNCTION__, __VA_ARGS__)
#else
#define debugLog(...) Engine::ContextLogger::log(__FUNCTION__, __VA_ARGS__)
#endif

class Engine final : public KeyboardListener
{
public:
	Engine();
	~Engine() override;

	// app
	void loadApp();

	// render/update
	void onPaint();
	void onUpdate();

	// window messages
	void onFocusGained();
	void onFocusLost();
	void onMinimized();
	void onMaximized();
	void onRestored();
	void onResolutionChange(Vector2 newResolution);
	void onDPIChange();
	void onShutdown();

	// hardcoded engine hotkeys
	void onKeyDown(KeyboardEvent &e) override;
	void onKeyUp(KeyboardEvent &) override { ; }
	void onChar(KeyboardEvent &) override { ; }

	// convenience functions (passthroughs)
	void shutdown();
	void restart();
	void focus();
	void center();
	void toggleFullscreen();
	void disableFullscreen();

	void showMessageInfo(const UString& title, const UString& message);
	void showMessageWarning(const UString& title, const UString& message);
	void showMessageError(const UString& title, const UString& message);
	void showMessageErrorFatal(const UString& title, const UString& message);

	// engine specifics
	void blackout() { m_bBlackout = true; }

public:
	// input devices
	[[nodiscard]] constexpr const std::vector<Mouse *> &getMice() const { return m_mice; }
	[[nodiscard]] constexpr const std::vector<Keyboard *> &getKeyboards() const { return m_keyboards; }

	// screen
	void requestResolutionChange(Vector2 newResolution);
	[[nodiscard]] constexpr const Vector2 &getScreenSize() const { return m_vScreenSize; }
	[[nodiscard]] inline int getScreenWidth() const { return (int)m_vScreenSize.x; }
	[[nodiscard]] inline int getScreenHeight() const { return (int)m_vScreenSize.y; }
	[[nodiscard]] constexpr const McRect &getScreenRect() const { return m_screenRect; }

	// timing
	void setFrameTime(double delta);

	template <typename T = double>
		requires(std::floating_point<T> || std::convertible_to<double, T>)
	[[nodiscard]] constexpr T getTime() const { return static_cast<T>(m_dTime); }
	template <typename T = double>
		requires(std::floating_point<T> || std::convertible_to<double, T>)
	[[nodiscard]] constexpr T getTimeRunning() const { return static_cast<T>(m_dRunTime); }
	template <typename T = double>
		requires(std::floating_point<T> || std::convertible_to<double, T>)
	[[nodiscard]] constexpr T getFrameTime() const { return static_cast<T>(m_dFrameTime); }

	// get real elapsed time since engine initialization
	// WARNING: not thread-safe!
	template <typename T = double>
		requires(std::floating_point<T> || std::convertible_to<double, T>)
	[[nodiscard]] T getLiveElapsedEngineTime() {
		m_timer->update();
		return static_cast<T>(m_timer->getElapsedTime());
	}

	[[nodiscard]] inline uint64_t getFrameCount() const { return m_iFrameCount; }
	// clang-format off
	// NOTE: if engine_throttle cvar is off, this will always return true
	[[nodiscard]] inline bool throttledShouldRun(unsigned int howManyVsyncFramesToWaitBetweenExecutions) { return (m_fVsyncFrameCounterTime == 0.0f) && !(m_iVsyncFrameCount % howManyVsyncFramesToWaitBetweenExecutions);}
	// clang-format on

	// vars
	[[nodiscard]] inline bool hasFocus() const { return m_bHasFocus; }
	[[nodiscard]] inline bool isDrawing() const { return m_bDrawing; }
	[[nodiscard]] inline bool isMinimized() const { return m_bIsMinimized; }

	// debugging/console
	void setConsole(Console *console) { m_console = console; }
	[[nodiscard]] inline ConsoleBox *getConsoleBox() const { return m_consoleBox; }
	[[nodiscard]] inline Console *getConsole() const { return m_console; }
	[[nodiscard]] inline CBaseUIContainer *getGUI() const { return m_guiContainer; }
	static void printVersion();

private:
	// input devices
	std::vector<Mouse *> m_mice;
	std::vector<Keyboard *> m_keyboards;
	std::vector<InputDevice *> m_inputDevices;

	// timing
	Timer *m_timer;
	double m_dTime;
	double m_dRunTime;
	uint64_t m_iFrameCount;
	uint8_t m_iVsyncFrameCount; // this will wrap quickly, and that's fine, it should be used as a dividend in a modular expression anyways
	float m_fVsyncFrameCounterTime;
	double m_dFrameTime;
	void onEngineThrottleChanged(float newVal);

	// primary screen
	Vector2 m_vScreenSize;
	Vector2 m_vNewScreenSize;
	McRect m_screenRect;
	bool m_bResolutionChange;

	// window
	bool m_bHasFocus;
	bool m_bIsMinimized;

	// engine gui, mostly for debugging
	CBaseUIContainer *m_guiContainer;
	VisualProfiler *m_visualProfiler;
	static ConsoleBox *m_consoleBox;
	static Console *m_console;

	// custom
	bool m_bBlackout;
	bool m_bIsRestarting;
	bool m_bDrawing;

	// math
	McMath *m_math;

public:
	class ContextLogger
	{
	public:
		// debug build shows full source location
		template <typename... Args>
		static void log(const std::source_location &loc, const char *func, fmt::format_string<Args...> fmt, Args &&...args)
		{
			auto contextPrefix =
			    fmt::format("[{}:{}:{}] [{}]: ", Environment::getFileNameFromFilePath(loc.file_name()), loc.line(), loc.column(), func);

			auto message = fmt::format(fmt, std::forward<Args>(args)...);
			Engine::logImpl(contextPrefix + message);
		}

		template <typename... Args>
		static void log(const std::source_location &loc, const char *func, Color color, fmt::format_string<Args...> fmt, Args &&...args)
		{
			auto contextPrefix =
			    fmt::format("[{}:{}:{}] [{}]: ", Environment::getFileNameFromFilePath(loc.file_name()), loc.line(), loc.column(), func);

			auto message = fmt::format(fmt, std::forward<Args>(args)...);
			Engine::logImpl(contextPrefix + message, color);
		}

		// release build only shows function name
		template <typename... Args>
		static void log(const char *func, fmt::format_string<Args...> fmt, Args &&...args)
		{
			auto contextPrefix = fmt::format("[{}] ", func);
			auto message = fmt::format(fmt, std::forward<Args>(args)...);
			Engine::logImpl(contextPrefix + message);
		}

		template <typename... Args>
		static void log(const char *func, Color color, fmt::format_string<Args...> fmt, Args &&...args)
		{
			auto contextPrefix = fmt::format("[{}] ", func);
			auto message = fmt::format(fmt, std::forward<Args>(args)...);
			Engine::logImpl(contextPrefix + message, color);
		}
	};
	template <typename... Args>
	static void logRaw(fmt::format_string<Args...> fmt, Args &&...args)
	{
		auto message = fmt::format(fmt, std::forward<Args>(args)...);
		Engine::logImpl(message);
	}
private:
	// logging stuff (implementation)
	static void logToConsole(std::optional<Color> color, const UString &message);

	static void logImpl(const std::string &message, Color color = rgb(255, 255, 255))
	{
		if constexpr (Env::cfg(OS::WINDOWS)) // hmm... odd bug with fmt::print (or mingw?), when the stdout isn't redirected to a file
		{
			if (color == rgb(255, 255, 255) || !Environment::isaTTY())
				printf("%s", message.c_str());
			else
				printf("%s", fmt::format(fmt::fg(fmt::rgb(color.R(), color.G(), color.B())), "{}", message).c_str());
		}
		else
		{
			if (color == rgb(255, 255, 255) || !Environment::isaTTY())
				fmt::print("{}", message);
			else
				fmt::print(fmt::fg(fmt::rgb(color.R(), color.G(), color.B())), "{}", message);
		}
		logToConsole(color, UString(message));
	}
};

extern std::unique_ptr<Mouse> mouse;
extern std::unique_ptr<Keyboard> keyboard;
extern std::unique_ptr<App> app;
extern std::unique_ptr<Graphics> g;
extern std::unique_ptr<SoundEngine> soundEngine;
extern std::unique_ptr<ResourceManager> resourceManager;
extern std::unique_ptr<NetworkHandler> networkHandler;
extern std::unique_ptr<AnimationHandler> animationHandler;
extern std::unique_ptr<SteamworksInterface> steam;
extern std::unique_ptr<DiscordInterface> discord;

extern Engine *engine;

#endif
