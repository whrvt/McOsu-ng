//================ Copyright (c) 2012, PG, All rights reserved. =================//
//
// Purpose:		core
//
// $NoKeywords: $engine
//===============================================================================//

#pragma once
#ifndef ENGINE_H
#define ENGINE_H

#include "cbase.h"

class App;
class Timer;
class Mouse;
class ConVar;
class Gamepad;
class Keyboard;
class InputDevice;
class SoundEngine;
class ContextMenu;
class NetworkHandler;
class OpenCLInterface;
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

class Engine
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
		template<typename... Args>
		static void log(const char* function, const char* fmt, Args... args)
		{
			std::string newFormat = formatWithContext(function, fmt);
			Engine::debugLog_(newFormat.c_str(), args...);
		}

		template<typename... Args>
		static void log(const char* function, Color color, const char* fmt, Args... args)
		{
			std::string newFormat = formatWithContext(function, fmt);
			Engine::debugLog_(color, newFormat.c_str(), args...);
		}
		
	private:
		static std::string formatWithContext(const char* function, const char* fmt)
		{
			std::ostringstream contextPrefix;
			contextPrefix << "[" << function << "] ";
			return contextPrefix.str() + fmt;
		}
	};

public:
	Engine(Environment *environment, const char *args = NULL);
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

	// primary mouse messages
	void onMouseRawMove(float xDelta, float yDelta, bool absolute = false, bool virtualDesktop = false);
	void onMouseRawMove(int xDelta, int yDelta, bool absolute = false, bool virtualDesktop = false);
	void onMouseWheelVertical(int delta);
	void onMouseWheelHorizontal(int delta);
	void onMouseLeftChange(bool mouseLeftDown);
	void onMouseMiddleChange(bool mouseMiddleDown);
	void onMouseRightChange(bool mouseRightDown);
	void onMouseButton4Change(bool mouse4down);
	void onMouseButton5Change(bool mouse5down);

	// primary keyboard messages
	void onKeyboardKeyDown(KEYCODE keyCode);
	void onKeyboardKeyUp(KEYCODE keyCode);
	void onKeyboardChar(KEYCODE charCode);

	// convenience functions (passthroughs)
	void shutdown();
	void restart();
	void sleep(unsigned int us);
	void focus();
	void center();
	void toggleFullscreen();
	void disableFullscreen();

	void showMessageInfo(UString title, UString message);
	void showMessageWarning(UString title, UString message);
	void showMessageError(UString title, UString message);
	void showMessageErrorFatal(UString title, UString message);

	// engine specifics
	void blackout() {m_bBlackout = true;}
	void addGamepad(Gamepad *gamepad);
	void removeGamepad(Gamepad *gamepad);

	// interfaces
	inline App *getApp() const {return m_app;}
	inline Graphics *getGraphics() const {return m_graphics;}
	inline SoundEngine *getSound() const {return m_sound;}
	inline ResourceManager *getResourceManager() const {return m_resourceManager;}
	inline Environment *getEnvironment() const {return m_environment;}
	inline NetworkHandler *getNetworkHandler() const {return m_networkHandler;}

	// input devices
	inline Mouse *getMouse() const {return m_mouse;}
	inline Keyboard *getKeyboard() const {return m_keyboard;}
	inline Gamepad *getGamepad() const {return m_gamepad;}
	inline const std::vector<Mouse*> &getMice() const {return m_mice;}
	inline const std::vector<Keyboard*> &getKeyboards() const {return m_keyboards;}
	inline const std::vector<Gamepad*> &getGamepads() const {return m_gamepads;}

	// screen
	void requestResolutionChange(Vector2 newResolution);
	inline Vector2 getScreenSize() const {return m_vScreenSize;}
	inline int getScreenWidth() const {return (int)m_vScreenSize.x;}
	inline int getScreenHeight() const {return (int)m_vScreenSize.y;}

	// vars
	void setFrameTime(double delta);
	inline double getTime() const {return m_dTime;}
	double const getTimeReal();
	inline double getTimeRunning() const {return m_dRunTime;}
	inline double getFrameTime() const {return m_dFrameTime;}
	inline unsigned long getFrameCount() const {return m_iFrameCount;}

	UString getArgs() const {return m_sArgs;}

	inline bool hasFocus() const {return m_bHasFocus;}
	inline bool isDrawing() const {return m_bDrawing;}
	inline bool isMinimized() const {return m_bIsMinimized;}

	// debugging/console
	void setConsole(Console *console) {m_console = console;}
	inline ConsoleBox *getConsoleBox() const {return m_consoleBox;}
	inline Console *getConsole() const {return m_console;}
	inline CBaseUIContainer *getGUI() const {return m_guiContainer;}

private:
	// interfaces
	App *m_app;
	Graphics *m_graphics;
	SoundEngine *m_sound;
	OpenCLInterface *m_openCL;
	OpenVRInterface *m_openVR;
	VulkanInterface *m_vulkan;
	ContextMenu *m_contextMenu;
	Environment *m_environment;
	NetworkHandler *m_networkHandler;
	ResourceManager *m_resourceManager;
	AnimationHandler *m_animationHandler;
	SteamworksInterface *m_steam;
	DiscordInterface *m_discord;

	// input devices
	Mouse *m_mouse;
	Keyboard *m_keyboard;
	Gamepad *m_gamepad;
	std::vector<Mouse*> m_mice;
	std::vector<Keyboard*> m_keyboards;
	std::vector<Gamepad*> m_gamepads;
	std::vector<InputDevice*> m_inputDevices;

	// timing
	Timer *m_timer;
	double m_dTime;
	double m_dRunTime;
	unsigned long m_iFrameCount;
	double m_dFrameTime;

	// primary screen
	Vector2 m_vScreenSize;
	Vector2 m_vNewScreenSize;
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
	UString m_sArgs;
	bool m_bBlackout;
	bool m_bDrawing;
	int m_iLoadingScreenDelay;
};

extern Engine *engine;

#ifdef _DEBUG
#include <source_location>
#define debugLog(...) Engine::ContextLogger::log(std::source_location::current().function_name(), __VA_ARGS__)
#else
#define debugLog(...) Engine::ContextLogger::log(__FUNCTION__, __VA_ARGS__)
#endif

#endif
