//======== Copyright (c) 2015-2018, PG & 2025, WH, All rights reserved. =========//
//
// Purpose:		top level interface for native OS calls
//
// $NoKeywords: $sdlenv
//===============================================================================//

#pragma once

#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "ConVar.h"
#include "Cursors.h"
#include "KeyboardEvent.h"
#include "cbase.h"
#include <SDL3/SDL.h>

#include <map>

class UString;
class Engine;
class Environment
{
public:
	Environment(int argc, char *argv[]);
	~Environment();

	void update();

	// engine/factory
	Graphics *createRenderer();

	// system
	void shutdown();
	void restart();
	[[nodiscard]] inline bool isRunning() const { return m_bRunning; }
	[[nodiscard]] static UString getExecutablePath();
	static void openURLInDefaultBrowser(const UString& url);

	[[nodiscard]] inline const std::unordered_map<UString, std::optional<UString>> &getLaunchArgs() const { return m_mArgMap; }
	[[nodiscard]] inline const std::vector<UString> &getCommandLine() const { return m_vCmdLine; }

	// returns at least 1
	[[nodiscard]] static inline int getLogicalCPUCount() { return SDL_GetNumLogicalCPUCores(); }

	// user
	[[nodiscard]] UString getUsername();
	[[nodiscard]] UString getUserDataPath();
	[[nodiscard]] UString getLocalDataPath();

	// file IO
	[[nodiscard]] static bool fileExists(UString &filename); // passthroughs to McFile
	[[nodiscard]] static bool directoryExists(UString &directoryName);
	[[nodiscard]] static bool fileExists(const UString &filename);
	[[nodiscard]] static bool directoryExists(const UString &directoryName);
	[[nodiscard]] static inline bool isaTTY() { return s_bIsATTY; } // is stdout a terminal

	[[nodiscard]] static bool createDirectory(const UString& directoryName);
	static bool renameFile(const UString& oldFileName, const UString& newFileName);
	static bool deleteFile(const UString& filePath);
	[[nodiscard]] static std::vector<UString> getFilesInFolder(const UString& folder);
	[[nodiscard]] static std::vector<UString> getFoldersInFolder(const UString& folder);
	[[nodiscard]] static std::vector<UString> getLogicalDrives();
	// returns an absolute (i.e. fully-qualified) filesystem path
	[[nodiscard]] static UString getFolderFromFilePath(const UString &filepath) noexcept;
	[[nodiscard]] static UString getFileExtensionFromFilePath(const UString& filepath, bool includeDot = false);
	[[nodiscard]] static UString getFileNameFromFilePath(const UString &filePath) noexcept;

	// clipboard
	[[nodiscard]] UString getClipBoardText();
	void setClipBoardText(const UString& text);

	// dialogs & message boxes
	void showMessageInfo(const UString& title, const UString& message) const;
	void showMessageWarning(const UString& title, const UString& message) const;
	void showMessageError(const UString& title, const UString& message) const;
	void showMessageErrorFatal(const UString& title, const UString& message) const;

	using FileDialogCallback = std::function<void(const std::vector<UString> &paths)>;
	void openFileWindow(FileDialogCallback callback, const char *filetypefilters, const UString& title, const UString& initialpath = "") const;
	void openFolderWindow(FileDialogCallback callback, const UString& initialpath = "") const;
	void openFileBrowser(const UString& title, UString initialpath = "") const noexcept;

	// window
	void focus();
	void center();
	void minimize();
	void maximize();
	void enableFullscreen();
	void disableFullscreen();
	void setWindowTitle(const UString& title);
	void setWindowPos(int x, int y);
	void setWindowSize(int width, int height);
	void setWindowResizable(bool resizable);
	void setFullscreenWindowedBorderless(bool fullscreenWindowedBorderless);
	void setMonitor(int monitor);
	[[nodiscard]] inline float getDisplayRefreshRate() const { return m_fDisplayHz; }
	[[nodiscard]] inline float getDisplayRefreshTime() const { return m_fDisplayHzSecs; }
	[[nodiscard]] HWND getHwnd() const;
	[[nodiscard]] Vector2 getWindowPos() const;
	[[nodiscard]] Vector2 getWindowSize() const;
	[[nodiscard]] int getMonitor() const;
	[[nodiscard]] std::map<unsigned int, McRect> getMonitors();
	[[nodiscard]] Vector2 getNativeScreenSize() const;
	[[nodiscard]] McRect getDesktopRect() const;
	[[nodiscard]] McRect getWindowRect() const;
	[[nodiscard]] bool isFullscreenWindowedBorderless() const { return m_bFullscreenWindowedBorderless; }
	[[nodiscard]] int getDPI() const;
	[[nodiscard]] float getDPIScale() const { return (float)getDPI() / 96.0f; }
	[[nodiscard]] inline bool isFullscreen() const { return m_bFullscreen; }
	[[nodiscard]] inline bool isWindowResizable() const { return m_bResizable; }

	// mouse
	[[nodiscard]] inline bool isCursorInWindow() const { return m_bIsCursorInsideWindow; }
	[[nodiscard]] inline bool isCursorVisible() const { return m_bCursorVisible; }
	[[nodiscard]] inline bool isCursorClipped() const { return m_bCursorClipped; }
	[[nodiscard]] Vector2 getMousePos() const { return m_vLastAbsMousePos; }
	[[nodiscard]] inline McRect getCursorClip() const { return m_cursorClip; }
	[[nodiscard]] inline CURSORTYPE getCursor() const { return m_cursorType; }
	void setCursor(CURSORTYPE cur);
	void setCursorVisible(bool visible);
	void setCursorClip(bool clip, McRect rect);
	void notifyWantRawInput(bool raw); // enable/disable OS-level rawinput
	inline void setMousePos(Vector2 pos)
	{
		m_vLastAbsMousePos = pos;
		setOSMousePos();
	}
	inline void setMousePos(float x, float y)
	{
		m_vLastAbsMousePos.x = x;
		m_vLastAbsMousePos.y = y;
		setOSMousePos();
	}

	// keyboard
	UString keyCodeToString(KEYCODE keyCode);
	void listenToTextInput(bool listen);

	// debug
	[[nodiscard]] inline bool envDebug() const { return m_bEnvDebug; }
protected:
	std::unordered_map<UString, std::optional<UString>> m_mArgMap;
	std::vector<UString> m_vCmdLine;
	Engine *initEngine();
	Engine *m_engine;

	SDL_Window *m_window;

	bool m_bRunning;
	bool m_bDrawing;

	bool m_bMinimized; // for fps_max_background
	bool m_bHasFocus;  // for fps_max_background

	inline void setOSMousePos() const { SDL_WarpMouseInWindow(m_window, m_vLastAbsMousePos.x, m_vLastAbsMousePos.y); }
	inline void setOSMousePos(Vector2 pos) const { SDL_WarpMouseInWindow(m_window, pos.x, pos.y); }

	// the absolute/relative mouse position from the most recent iteration of the event loop
	// relative only useful if raw input is enabled, value is undefined/garbage otherwise
	Vector2 m_vLastAbsMousePos;
	Vector2 m_vLastRelMousePos;

	// cache
	UString m_sUsername;
	UString m_sProgDataPath;
	UString m_sAppDataPath;
	HWND m_hwnd;

	// logging
	inline bool envDebug(bool enable)
	{
		m_bEnvDebug = enable;
		return m_bEnvDebug;
	}
	void onLogLevelChange(float newval);
	bool m_bEnvDebug;

	static bool s_bIsATTY;

	// monitors
	void initMonitors(bool force = false);
	std::map<unsigned int, McRect> m_mMonitors;
	float m_fDisplayHz;
	float m_fDisplayHzSecs;

	// window
	bool m_bResizable;
	bool m_bFullscreen;
	bool m_bFullscreenWindowedBorderless;
	inline void onFullscreenWindowBorderlessChange(float newValue) { setFullscreenWindowedBorderless(!!static_cast<int>(newValue)); }
	inline void onMonitorChange(float oldValue, float newValue) { if (oldValue != newValue) setMonitor(static_cast<int>(newValue)); }

	// mouse
	bool m_bIsCursorInsideWindow;
	bool m_bCursorVisible;
	bool m_bCursorClipped;
	McRect m_cursorClip;
	CURSORTYPE m_cursorType;
	std::map<CURSORTYPE, SDL_Cursor *> m_mCursorIcons;

	// clipboard
	UString m_sCurrClipboardText;

	// misc
	void initCursors();

private:
	// static callbacks/helpers
	struct FileDialogCallbackData
	{
		FileDialogCallback callback;
	};
	static void sdlFileDialogCallback(void *userdata, const char *const *filelist, int filter);

	static std::vector<UString> enumerateDirectory(const char *pathToEnum, SDL_PathType type); // code sharing for getFilesInFolder/getFoldersInFolder
	static UString getThingFromPathHelper(UString path, bool folder) noexcept; // code sharing for getFolderFromFilePath/getFileNameFromFilePath

	static void winSortInPlace(std::vector<UString> &toSort); // for sorting a list kinda in the order windows' explorer would
};

extern Environment *env;

#endif
