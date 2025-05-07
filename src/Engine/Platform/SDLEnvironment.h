//========== Copyright (c) 2018, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		SDL environment functionality
//
// $NoKeywords: $sdlenv
//===============================================================================//

#pragma once

#ifndef SDLENVIRONMENT_H
#define SDLENVIRONMENT_H

#include "Environment.h"

#include "cbase.h"
#include <SDL3/SDL.h>

#include "ConVar.h"
#include "Engine.h"

#include <map>

extern ConVar mouse_raw_input;

class UString;
class Engine;
class SDLEnvironment : public Environment
{
public:
	SDLEnvironment();
	~SDLEnvironment() override;

	int main(int argc, char *argv[]);

	void update() override;

	// engine/factory
	Graphics *createRenderer() override;
	ContextMenu *createContextMenu() override;

	// system
	void shutdown() override;
	void restart() override;
	void sleep(unsigned int us) override;
	[[nodiscard]] UString getExecutablePath() const override;
	void openURLInDefaultBrowser(UString url) const override;
	// returns at least 1
	[[nodiscard]] inline int getLogicalCPUCount() const override { return SDL_GetNumLogicalCPUCores(); }

	// user
	[[nodiscard]] UString getUsername() override;
	[[nodiscard]] UString getUserDataPath() const override;

	// file IO
	[[nodiscard]] bool fileExists(UString filename) const override;
	[[nodiscard]] bool directoryExists(UString directoryName) const override;
	[[nodiscard]] bool createDirectory(UString directoryName) const override;
	bool renameFile(UString oldFileName, UString newFileName) override;
	bool deleteFile(UString filePath) override;
	[[nodiscard]] std::vector<UString> getFilesInFolder(UString folder) const override;
	[[nodiscard]] std::vector<UString> getFoldersInFolder(UString folder) const override;
	[[nodiscard]] std::vector<UString> getLogicalDrives() const override;
	[[nodiscard]] UString getFolderFromFilePath(UString filepath) const override;
	[[nodiscard]] UString getFileExtensionFromFilePath(UString filepath, bool includeDot = false) const override;
	[[nodiscard]] UString getFileNameFromFilePath(UString filePath) const override;

	// clipboard
	[[nodiscard]] UString getClipBoardText() override;
	void setClipBoardText(UString text) override;

	// dialogs & message boxes
	void showMessageInfo(UString title, UString message) const override;
	void showMessageWarning(UString title, UString message) const override;
	void showMessageError(UString title, UString message) const override;
	void showMessageErrorFatal(UString title, UString message) const override;
	UString openFileWindow(const char *filetypefilters, UString title, UString initialpath) const override;
	[[nodiscard]] UString openFolderWindow(UString title, UString initialpath) const override;

	// window
	void focus() override;
	void center() override;
	void minimize() override;
	void maximize() override;
	void enableFullscreen() override;
	void disableFullscreen() override;
	void setWindowTitle(UString title) override;
	void setWindowPos(int x, int y) override;
	void setWindowSize(int width, int height) override;
	void setWindowResizable(bool resizable) override;
	void setWindowGhostCorporeal(bool corporeal) override;
	void setMonitor(int monitor) override;
	[[nodiscard]] HWND getHwnd() const override;
	[[nodiscard]] Vector2 getWindowPos() const override;
	[[nodiscard]] Vector2 getWindowSize() const override;
	[[nodiscard]] int getMonitor() const override;
	[[nodiscard]] inline std::vector<McRect> getMonitors() const override { return m_vMonitors; }
	[[nodiscard]] Vector2 getNativeScreenSize() const override;
	[[nodiscard]] McRect getVirtualScreenRect() const override;
	[[nodiscard]] McRect getDesktopRect() const override;
	[[nodiscard]] McRect getWindowRect() const override;
	[[nodiscard]] int getDPI() const override;
	[[nodiscard]] inline bool isFullscreen() const override { return m_bFullscreen; }
	[[nodiscard]] inline bool isWindowResizable() const override { return m_bResizable; }

	// mouse
	[[nodiscard]] inline bool isCursorInWindow() const override { return m_bIsCursorInsideWindow; }
	[[nodiscard]] inline bool isCursorVisible() const override { return m_bCursorVisible; }
	[[nodiscard]] inline bool isCursorClipped() const override { return m_bCursorClipped; }
	[[nodiscard]] Vector2 getMousePos() const override;
	[[nodiscard]] inline McRect getCursorClip() const override { return m_cursorClip; }
	[[nodiscard]] inline CURSORTYPE getCursor() const override { return m_cursorType; }
	void setCursor(CURSORTYPE cur) override;
	void setCursorVisible(bool visible) override;
	void setCursorClip(bool clip, McRect rect) override;
	inline void setMousePos(Vector2 pos) override
	{
		m_vLastAbsMousePos = pos;
		setCursorPosition();
	}
	inline void setMousePos(float x, float y) override
	{
		m_vLastAbsMousePos.x = x;
		m_vLastAbsMousePos.y = y;
		setCursorPosition();
	}
	// keyboard
	UString keyCodeToString(KEYCODE keyCode) override;
	void listenToTextInput(bool listen) override;

protected:
	inline void setCursorPosition() const override { SDL_WarpMouseInWindow(m_window, m_vLastAbsMousePos.x, m_vLastAbsMousePos.y); }
	inline void setCursorPosition(Vector2 pos) const override { SDL_WarpMouseInWindow(m_window, pos.x, pos.y); }

private:
	// logging
	[[nodiscard]] inline bool sdlDebug() const { return m_sdlDebug; }
	inline bool sdlDebug(bool enable)
	{
		m_sdlDebug = enable;
		return m_sdlDebug;
	}
	void onLogLevelChange(float newval);

	Engine *m_engine;

	static constexpr bool m_bUpdate = true;
	static constexpr bool m_bDraw = true;

	bool m_bRunning;
	bool m_bDrawing;

	bool m_bMinimized; // for fps_max_background
	bool m_bHasFocus;  // for fps_max_background

	bool m_bIsRawInput;

	// cache
	UString m_sUsername;
	HWND m_hwnd;

	// logging
	bool m_sdlDebug;

	// monitors
	std::vector<McRect> m_vMonitors;

	// window
	SDL_Window *m_window;
	bool m_bResizable;
	bool m_bFullscreen;

	// mouse
	bool m_bIsCursorInsideWindow;
	bool m_bCursorVisible;
	bool m_bCursorClipped;
	McRect m_cursorClip;
	CURSORTYPE m_cursorType;
	std::map<CURSORTYPE, SDL_Cursor *> m_mCursorIcons;

	// the absolute/relative mouse position from the most recent iteration of the event loop
	// relative only useful if raw input is enabled, value is undefined/garbage otherwise
	Vector2 m_vLastAbsMousePos;
	Vector2 m_vLastRelMousePos;

	void onRawInputChange(float newval);
	void setRawInput(bool on);

	// clipboard
	const char *m_sPrevClipboardTextSDL;

private:
	// SDL dialog callbacks/helpers
	struct FileDialogState
	{
		bool done;
		UString result;
	};
	static FileDialogState s_fileDialogState;
	static void fileDialogCallback(void *userdata, const char *const *filelist, int filter);
};

#endif
