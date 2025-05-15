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

class ContextMenu;
class UString;
class Engine;
class Environment
{
public:
	Environment();
	~Environment();

	void update();

	// engine/factory
	Graphics *createRenderer();
	ContextMenu *createContextMenu();

	// system
	void shutdown();
	void restart();
	[[nodiscard]] UString getExecutablePath() const;
	void openURLInDefaultBrowser(UString url) const;
	// returns at least 1
	[[nodiscard]] inline int getLogicalCPUCount() const { return SDL_GetNumLogicalCPUCores(); }

	// user
	[[nodiscard]] UString getUsername();
	[[nodiscard]] UString getUserDataPath();
	[[nodiscard]] UString getLocalDataPath();

	// file IO
	[[nodiscard]] bool fileExists(UString filename) const;
	[[nodiscard]] bool directoryExists(UString directoryName) const;
	[[nodiscard]] bool createDirectory(UString directoryName) const;
	bool renameFile(UString oldFileName, UString newFileName);
	bool deleteFile(UString filePath);
	[[nodiscard]] std::vector<UString> getFilesInFolder(UString folder) const;
	[[nodiscard]] std::vector<UString> getFoldersInFolder(UString folder) const;
	[[nodiscard]] std::vector<UString> getLogicalDrives() const;
	[[nodiscard]] UString getFolderFromFilePath(UString filepath) const;
	[[nodiscard]] UString getFileExtensionFromFilePath(UString filepath, bool includeDot = false) const;
	[[nodiscard]] UString getFileNameFromFilePath(UString filePath) const;

	// clipboard
	[[nodiscard]] UString getClipBoardText();
	void setClipBoardText(UString text);

	// dialogs & message boxes
	void showMessageInfo(UString title, UString message) const;
	void showMessageWarning(UString title, UString message) const;
	void showMessageError(UString title, UString message) const;
	void showMessageErrorFatal(UString title, UString message) const;

	using FileDialogCallback = std::function<void(const std::vector<UString> &paths)>;
	void openFileWindow(FileDialogCallback callback, const char *filetypefilters, UString title, UString initialpath = "") const;
	void openFolderWindow(FileDialogCallback callback, UString initialpath = "") const;

	// window
	void focus();
	void center();
	void minimize();
	void maximize();
	void enableFullscreen();
	void disableFullscreen();
	void setWindowTitle(UString title);
	void setWindowPos(int x, int y);
	void setWindowSize(int width, int height);
	void setWindowResizable(bool resizable);
	void setWindowGhostCorporeal(bool corporeal);
	void setFullscreenWindowedBorderless(bool fullscreenWindowedBorderless);
	void setMonitor(int monitor);
	[[nodiscard]] HWND getHwnd() const;
	[[nodiscard]] Vector2 getWindowPos() const;
	[[nodiscard]] Vector2 getWindowSize() const;
	[[nodiscard]] int getMonitor() const;
	[[nodiscard]] inline std::vector<McRect> getMonitors() const { return m_vMonitors; }
	[[nodiscard]] Vector2 getNativeScreenSize() const;
	[[nodiscard]] McRect getVirtualScreenRect() const;
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
	[[nodiscard]] Vector2 getMousePos() const;
	[[nodiscard]] inline McRect getCursorClip() const { return m_cursorClip; }
	[[nodiscard]] inline CURSORTYPE getCursor() const { return m_cursorType; }
	void setCursor(CURSORTYPE cur);
	void setCursorVisible(bool visible);
	void setCursorClip(bool clip, McRect rect);
	inline void setMousePos(Vector2 pos)
	{
		m_vLastAbsMousePos = pos;
		setCursorPosition();
	}
	inline void setMousePos(float x, float y)
	{
		m_vLastAbsMousePos.x = x;
		m_vLastAbsMousePos.y = y;
		setCursorPosition();
	}

	// keyboard
	UString keyCodeToString(KEYCODE keyCode);
	void listenToTextInput(bool listen);

protected:
	inline void setCursorPosition() const { SDL_WarpMouseInWindow(m_window, m_vLastAbsMousePos.x, m_vLastAbsMousePos.y); }
	inline void setCursorPosition(Vector2 pos) const { SDL_WarpMouseInWindow(m_window, pos.x, pos.y); }

	Engine *m_engine;

	SDL_Window *m_window;

	bool m_bRunning;
	bool m_bDrawing;

	bool m_bMinimized; // for fps_max_background
	bool m_bHasFocus;  // for fps_max_background

	// the absolute/relative mouse position from the most recent iteration of the event loop
	// relative only useful if raw input is enabled, value is undefined/garbage otherwise
	Vector2 m_vLastAbsMousePos;
	Vector2 m_vLastRelMousePos;

private:
	// logging
	[[nodiscard]] inline bool sdlDebug() const { return m_sdlDebug; }
	inline bool sdlDebug(bool enable)
	{
		m_sdlDebug = enable;
		return m_sdlDebug;
	}
	static ConVar *debug_env;
	void onLogLevelChange(float newval);

	bool m_bIsRawInput;

	// cache
	UString m_sUsername;
	UString m_sProgDataPath;
	UString m_sAppDataPath;
	HWND m_hwnd;

	// logging
	bool m_sdlDebug;

	// monitors
	std::vector<McRect> m_vMonitors;

	// window
	bool m_bResizable;
	bool m_bFullscreen;
	bool m_bFullscreenWindowedBorderless;
	inline void onFullscreenWindowBorderlessChange(UString oldValue, UString newValue) { setFullscreenWindowedBorderless(newValue.toFloat() > 0.0f); }
	inline void onMonitorChange(UString oldValue, UString newValue) { setMonitor(newValue.toInt()); }

	// mouse
	bool m_bIsCursorInsideWindow;
	bool m_bCursorVisible;
	bool m_bCursorClipped;
	McRect m_cursorClip;
	CURSORTYPE m_cursorType;
	std::map<CURSORTYPE, SDL_Cursor *> m_mCursorIcons;

	void onRawInputChange(float newval);
	void setRawInput(bool on);

	// clipboard
	const char *m_sPrevClipboardTextSDL;

	// misc
	inline void onProcessPriorityChange(float newValue) { SDL_SetCurrentThreadPriority(!!static_cast<int>(newValue) ? SDL_THREAD_PRIORITY_HIGH : SDL_THREAD_PRIORITY_NORMAL); }

private:
	// SDL dialog callbacks/helpers
	struct FileDialogCallbackData
	{
		FileDialogCallback callback;
	};
    static void sdlFileDialogCallback(void* userdata, const char* const* filelist, int filter);
};

extern Environment *env;

#endif
