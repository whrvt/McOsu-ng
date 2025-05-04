//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		top level interface for native OS calls
//
// $NoKeywords: $env
//===============================================================================//

#pragma once
#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "cbase.h"

#include "Cursors.h"
#include "KeyboardEvent.h"

class ContextMenu;
class Environment
{

public:
	Environment();
	virtual ~Environment() {;}

	virtual void update() {;}

	// engine/factory
	virtual Graphics *createRenderer() = 0;
	virtual ContextMenu *createContextMenu() = 0;

	virtual void shutdown() = 0;
	virtual void restart() = 0;
	virtual void sleep(unsigned int us) = 0;
	[[nodiscard]] virtual UString getExecutablePath() const = 0;
	virtual void openURLInDefaultBrowser(UString url) const = 0;
	[[nodiscard]] virtual int getLogicalCPUCount() const = 0;

	// user
	[[nodiscard]] virtual UString getUsername() const = 0;
	[[nodiscard]] virtual UString getUserDataPath() const = 0;

	// file IO
	[[nodiscard]] virtual bool fileExists(UString fileName) const = 0;
	[[nodiscard]] virtual bool directoryExists(UString directoryName) const = 0;
	[[nodiscard]] virtual bool createDirectory(UString directoryName) const = 0;
	virtual bool renameFile(UString oldFileName, UString newFileName) = 0;
	virtual bool deleteFile(UString filePath) = 0;
	[[nodiscard]] virtual std::vector<UString> getFilesInFolder(UString folder) const = 0;
	[[nodiscard]] virtual std::vector<UString> getFoldersInFolder(UString folder) const = 0;
	[[nodiscard]] virtual std::vector<UString> getLogicalDrives() const = 0;
	[[nodiscard]] virtual UString getFolderFromFilePath(UString filepath) const = 0;
	[[nodiscard]] virtual UString getFileExtensionFromFilePath(UString filepath, bool includeDot = false) const = 0;
	[[nodiscard]] virtual UString getFileNameFromFilePath(UString filePath) const = 0;

	// clipboard
	[[nodiscard]] virtual UString getClipBoardText() = 0;
	virtual void setClipBoardText(UString text) = 0;

	// dialogs & message boxes
	virtual void showMessageInfo(UString title, UString message) const = 0;
	virtual void showMessageWarning(UString title, UString message) const = 0;
	virtual void showMessageError(UString title, UString message) const = 0;
	virtual void showMessageErrorFatal(UString title, UString message) const = 0;
	virtual UString openFileWindow(const char *filetypefilters, UString title, UString initialpath) const = 0;
	[[nodiscard]] virtual UString openFolderWindow(UString title, UString initialpath) const = 0;

	// window
	virtual void focus() = 0;
	virtual void center() = 0;
	virtual void minimize() = 0;
	virtual void maximize() = 0;
	virtual void enableFullscreen() = 0;
	virtual void disableFullscreen() = 0;
	virtual void setWindowTitle(UString title) = 0;
	virtual void setWindowPos(int x, int y) = 0;
	virtual void setWindowSize(int width, int height) = 0;
	virtual void setWindowResizable(bool resizable) = 0;
	virtual void setWindowGhostCorporeal(bool corporeal) = 0;
	virtual void setMonitor(int monitor) = 0;
	[[nodiscard]] virtual Vector2 getWindowPos() const = 0;
	[[nodiscard]] virtual Vector2 getWindowSize() const = 0;
	[[nodiscard]] virtual int getMonitor() const = 0;
	[[nodiscard]] virtual std::vector<McRect> getMonitors() const = 0;
	[[nodiscard]] virtual Vector2 getNativeScreenSize() const = 0;
	[[nodiscard]] virtual McRect getVirtualScreenRect() const = 0;
	[[nodiscard]] virtual McRect getDesktopRect() const = 0;
	[[nodiscard]] virtual int getDPI() const = 0;
	[[nodiscard]] virtual bool isFullscreen() const = 0;
	[[nodiscard]] virtual bool isWindowResizable() const = 0;

	// mouse
	[[nodiscard]] virtual bool isCursorInWindow() const = 0;
	[[nodiscard]] virtual bool isCursorVisible() const = 0;
	[[nodiscard]] virtual bool isCursorClipped() const = 0;
	[[nodiscard]] virtual Vector2 getMousePos() const = 0;
	[[nodiscard]] virtual McRect getCursorClip() const = 0;
	[[nodiscard]] virtual CURSORTYPE getCursor() const = 0;
	virtual void setCursor(CURSORTYPE cur) = 0;
	virtual void setCursorVisible(bool visible) = 0;
	template <typename T = float>
	inline void setMousePos(T x, T y)
	{
		m_vLastAbsMousePos.x = static_cast<float>(x);
		m_vLastAbsMousePos.y = static_cast<float>(y);
		setCursorPosition();
	}
	virtual void setCursorClip(bool clip, McRect rect) = 0;

	// keyboard
	virtual UString keyCodeToString(KEYCODE keyCode) = 0;
	virtual void listenToTextInput(bool listen) = 0;

public:
	// built-in convenience

	// window
	virtual void setFullscreenWindowedBorderless(bool fullscreenWindowedBorderless);
	virtual bool isFullscreenWindowedBorderless() {return m_bFullscreenWindowedBorderless;}
	virtual float getDPIScale() {return (float)getDPI() / 96.0f;}

protected:
	virtual void setCursorPosition() const = 0;
	virtual void setCursorPosition(Vector2 pos) const = 0;

	static ConVar *debug_env;

	bool m_bFullscreenWindowedBorderless;

	// the absolute/relative mouse position from the most recent iteration of the event loop
	// relative only useful if raw input is enabled, value is undefined/garbage otherwise
	Vector2 m_vLastAbsMousePos{};
	Vector2 m_vLastRelMousePos{};
};

extern Environment *env;

#endif
