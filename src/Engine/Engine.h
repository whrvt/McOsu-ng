//================ Copyright (c) 2012, PG, All rights reserved. =================//
//
// Purpose:		core
//
// $NoKeywords: $engine
//===============================================================================//

#pragma once
#ifndef ENGINE_H
#define ENGINE_H

#include "KeyboardListener.h"
#include "McMath.h"
#include "Timing.h"
#include "cbase.h"

#include <source_location>

#ifdef _DEBUG
#define debugLog(...) Engine::ContextLogger::log(std::source_location::current(), __VA_ARGS__)
#else
#define debugLog(...) Engine::ContextLogger::log(__FUNCTION__, __VA_ARGS__)
#endif

class App;
class Mouse;
class ConVar;
class Keyboard;
class InputDevice;
class SoundEngine;
class ContextMenu;
class NetworkHandler;
class OpenVRInterface;
class VulkanInterface;
class ResourceManager;
class AnimationHandler;
class SteamworksInterface;
class DiscordInterface;

class CBaseUIContainer;
class VisualProfiler;
class ConsoleBox;
class Console;

class Engine final : public KeyboardListener
{
public:
	static void debugLog_(const char *fmt, va_list args);
	static void debugLog_(Color color, const char *fmt, va_list args);
	static void debugLog_(const char *fmt, ...);
	static void debugLog_(Color color, const char *fmt, ...);

	// this is messy, but it's a way to add context without changing the public debugLog interface
	class ContextLogger
	{
	public:
		template <class T, typename... Args>
		static void log(T function, const char *fmt, Args... args)
		{
			std::string newFormat = formatWithContext(function, fmt);
			Engine::debugLog_(newFormat.c_str(), args...);
		}

		template <class T, typename... Args>
		static void log(T function, Color color, const char *fmt, Args... args)
		{
			std::string newFormat = formatWithContext(function, fmt);
			Engine::debugLog_(color, newFormat.c_str(), args...);
		}

	private:
		template <typename... Args>
		static std::string formatWithContext(const std::source_location func, const char *fmt)
		{
			std::ostringstream contextPrefix;
			contextPrefix << "[" << Environment::getFileNameFromFilePath(func.file_name()).toUtf8() << ":" << func.line() << ":" << func.column() << "] " << "[" << func.function_name() << "]: ";
			return contextPrefix.str() + fmt;
		}
		template <typename... Args>
		static std::string formatWithContext(const char *func, const char *fmt)
		{
			std::ostringstream contextPrefix;
			contextPrefix << "[" << func << "] ";

			return contextPrefix.str() + fmt;
		}
	};

public:
	Engine();
	~Engine();

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

	void showMessageInfo(UString title, UString message);
	void showMessageWarning(UString title, UString message);
	void showMessageError(UString title, UString message);
	void showMessageErrorFatal(UString title, UString message);

	// engine specifics
	void blackout() { m_bBlackout = true; }

private:
	// singleton interface/instance decls
	static std::unique_ptr<Mouse> s_mouseInstance;
	static std::unique_ptr<Keyboard> s_keyboardInstance;
	static std::unique_ptr<App> s_appInstance;
	static std::unique_ptr<Graphics> s_graphicsInstance;
	static std::unique_ptr<SoundEngine> s_soundEngineInstance;
	static std::unique_ptr<ResourceManager> s_resourceManagerInstance;
	static std::unique_ptr<NetworkHandler> s_networkHandlerInstance;
	static std::unique_ptr<OpenVRInterface> s_openVRInstance;
	static std::unique_ptr<VulkanInterface> s_vulkanInstance;
	static std::unique_ptr<ContextMenu> s_contextMenuInstance;
	static std::unique_ptr<AnimationHandler> s_animationHandlerInstance;
	static std::unique_ptr<SteamworksInterface> s_steamInstance;
	static std::unique_ptr<DiscordInterface> s_discordInstance;

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
	[[nodiscard]] inline double getTime() const { return m_dTime; }
	[[nodiscard]] inline double getTimeRunning() const { return m_dRunTime; }
	[[nodiscard]] inline double getFrameTime() const { return m_dFrameTime; }
	[[nodiscard]] inline unsigned long getFrameCount() const { return m_iFrameCount; }
	[[nodiscard]] inline bool vsyncFrame() const {return m_fFrameThrottleTime > 0;}

	// vars
	[[nodiscard]] inline bool hasFocus() const { return m_bHasFocus; }
	[[nodiscard]] inline bool isDrawing() const { return m_bDrawing; }
	[[nodiscard]] inline bool isMinimized() const { return m_bIsMinimized; }

	// debugging/console
	void setConsole(Console *console) { m_console = console; }
	[[nodiscard]] inline ConsoleBox *getConsoleBox() const { return m_consoleBox; }
	[[nodiscard]] inline Console *getConsole() const { return m_console; }
	[[nodiscard]] inline CBaseUIContainer *getGUI() const { return m_guiContainer; }

private:
	// input devices
	std::vector<Mouse *> m_mice;
	std::vector<Keyboard *> m_keyboards;
	std::vector<InputDevice *> m_inputDevices;

	// timing
	Timer *m_timer;
	double m_dTime;
	double m_dRunTime;
	unsigned long m_iFrameCount;
	double m_dFrameTime;
	float m_fFrameThrottleTime;
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
};

extern Mouse *mouse;
extern Keyboard *keyboard;
extern App *app;
extern Graphics *graphics;
extern SoundEngine *soundEngine;
extern ResourceManager *resourceManager;
extern NetworkHandler *networkHandler;
extern OpenVRInterface *openVR;
extern VulkanInterface *vulkan;
extern ContextMenu *contextMenu;
extern AnimationHandler *animationHandler;
extern SteamworksInterface *steam;
extern DiscordInterface *discord;

extern Engine *engine;

#endif
