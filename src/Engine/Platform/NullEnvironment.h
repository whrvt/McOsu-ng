//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		empty environment, for debugging and new OS implementations
//
// $NoKeywords: $ne
//===============================================================================//

#pragma once
#ifndef NULLENVIRONMENT_H
#define NULLENVIRONMENT_H

#include "Environment.h"

class NullEnvironment : public Environment
{
public:
	NullEnvironment();
	~NullEnvironment() override {;}

	// engine/factory
	Graphics *createRenderer() override;
	ContextMenu *createContextMenu() override;

	// system
	void shutdown() override;
	void restart() override;
	void sleep(unsigned int us) override {;}
	[[nodiscard]] UString getExecutablePath() const override {return "";}
	void openURLInDefaultBrowser(UString url) const override {;}
	[[nodiscard]] inline int getLogicalCPUCount() const override {return 1;}

	// user
	[[nodiscard]] UString getUsername() const override {return "<NULL>";}
	[[nodiscard]] UString getUserDataPath() const override {return "<NULL>";}

	// file IO
	[[nodiscard]] bool fileExists(UString filename) const override {return false;}
	[[nodiscard]] bool directoryExists(UString directoryName) const override {return false;}
	bool createDirectory(UString directoryName) override {return false;}
	bool renameFile(UString oldFileName, UString newFileName) override {return false;}
	bool deleteFile(UString filePath) override {return false;}
	[[nodiscard]] std::vector<UString> getFilesInFolder(UString folder) const override {return {};}
	[[nodiscard]] std::vector<UString> getFoldersInFolder(UString folder) const override {return {};}
	[[nodiscard]] std::vector<UString> getLogicalDrives() const override {return {};}
	[[nodiscard]] UString getFolderFromFilePath(UString filepath) const override {return "";}
	[[nodiscard]] UString getFileExtensionFromFilePath(UString filepath, bool includeDot = false) const override {return "";}
	[[nodiscard]] UString getFileNameFromFilePath(UString filePath) const override {return "";}

	// clipboard
	[[nodiscard]] UString getClipBoardText() override {return "";}
	void setClipBoardText(UString text) override {;}

	// dialogs & message boxes
	void showMessageInfo(UString title, UString message) const override {;}
	void showMessageWarning(UString title, UString message) const override {;}
	void showMessageError(UString title, UString message) const override {;}
	void showMessageErrorFatal(UString title, UString message) const override {;}
	UString openFileWindow(const char *filetypefilters, UString title, UString initialpath) const override {return "";}
	[[nodiscard]] UString openFolderWindow(UString title, UString initialpath) const override {return "";}

	// window
	void focus() override {;}
	void center() override {;}
	void minimize() override {;}
	void maximize() override {;}
	void enableFullscreen() override {;}
	void disableFullscreen() override {;}
	void setWindowTitle(UString title) override {;}
	void setWindowPos(int x, int y) override {;}
	void setWindowSize(int width, int height) override {;}
	void setWindowResizable(bool resizable) override {;}
	void setWindowGhostCorporeal(bool corporeal) override {;}
	void setMonitor(int monitor) override {;}
	[[nodiscard]] Vector2 getWindowPos() const override {return {0, 0};}
	[[nodiscard]] Vector2 getWindowSize() const override {return {1280, 720};}
	[[nodiscard]] int getMonitor() const override {return 0;}
	[[nodiscard]] std::vector<McRect> getMonitors() const override {return {};}
	[[nodiscard]] Vector2 getNativeScreenSize() const override {return {1920, 1080};}
	[[nodiscard]] McRect getVirtualScreenRect() const override {return {0, 0, 1920, 1080};}
	[[nodiscard]] McRect getDesktopRect() const override {return {0, 0, 1920, 1080};}
	[[nodiscard]] int getDPI() const override {return 96;}
	[[nodiscard]] bool isFullscreen() const override {return false;}
	[[nodiscard]] bool isWindowResizable() const override {return true;}

	// mouse
	[[nodiscard]] bool isCursorInWindow() const override {return true;}
	[[nodiscard]] bool isCursorVisible() const override {return true;}
	[[nodiscard]] bool isCursorClipped() const override {return false;}
	[[nodiscard]] Vector2 getMousePos() const override {return {0, 0};}
	[[nodiscard]] McRect getCursorClip() const override {return {0, 0, 0, 0};}
	[[nodiscard]] CURSORTYPE getCursor() const override {return CURSORTYPE::CURSOR_NORMAL;}
	void setCursor(CURSORTYPE cur) override {;}
	void setCursorVisible(bool visible) override {;}
	void setCursorClip(bool clip, McRect rect) override {;}

	// keyboard
	UString keyCodeToString(KEYCODE keyCode) override {return UString::format("%lu", keyCode);}
	void listenToTextInput(bool) override {;}

protected:
	void setCursorPosition() const override {;}

private:
	bool m_bRunning{};
};

#endif
