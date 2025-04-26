//================ Copyright (c) 2018, PG, All rights reserved. =================//
//
// Purpose:		SDL ("partial", SDL does not provide all functions!)
//
// $NoKeywords: $sdlenv
//===============================================================================//

#pragma once
#ifndef SDLENVIRONMENT_H
#define SDLENVIRONMENT_H

#include "Environment.h"

#ifdef MCENGINE_FEATURE_SDL

#include "cbase.h"
#include <SDL3/SDL.h>

class SDLEnvironment : public Environment
{
public:
	SDLEnvironment(SDL_Window *window);
	virtual ~SDLEnvironment() {;}

	virtual void update();

	// engine/factory
	virtual Graphics *createRenderer();
	virtual ContextMenu *createContextMenu();

	// system
	virtual void shutdown();
	virtual void restart();
	virtual void sleep(unsigned int us); // NOTE: inaccurate
	virtual UString getExecutablePath();
	virtual void openURLInDefaultBrowser(UString url); // NOTE: non-SDL

	// user
	virtual UString getUsername(); // NOTE: non-SDL
	virtual UString getUserDataPath();

	// file IO
	virtual bool fileExists(UString filename);
	virtual bool directoryExists(UString directoryName); // NOTE: non-SDL
	virtual bool createDirectory(UString directoryName); // NOTE: non-SDL
	virtual bool renameFile(UString oldFileName, UString newFileName);
	virtual bool deleteFile(UString filePath);
	virtual std::vector<UString> getFilesInFolder(UString folder);		// NOTE: non-SDL
	virtual std::vector<UString> getFoldersInFolder(UString folder);	// NOTE: non-SDL
	virtual std::vector<UString> getLogicalDrives();					// NOTE: non-SDL
	virtual UString getFolderFromFilePath(UString filepath);			// NOTE: non-SDL
	virtual UString getFileExtensionFromFilePath(UString filepath, bool includeDot = false);
	virtual UString getFileNameFromFilePath(UString filePath);			// NOTE: non-SDL

	// clipboard
	virtual UString getClipBoardText();
	virtual void setClipBoardText(UString text);

	// dialogs & message boxes
	virtual void showMessageInfo(UString title, UString message);
	virtual void showMessageWarning(UString title, UString message);
	virtual void showMessageError(UString title, UString message);
	virtual void showMessageErrorFatal(UString title, UString message);
	virtual UString openFileWindow(const char *filetypefilters, UString title, UString initialpath);	// NOTE: non-SDL
	virtual UString openFolderWindow(UString title, UString initialpath);								// NOTE: non-SDL

	// window
	virtual void focus();
	virtual void center();
	virtual void minimize();
	virtual void maximize();
	virtual void enableFullscreen();
	virtual void disableFullscreen();
	virtual void setWindowTitle(UString title);
	virtual void setWindowPos(int x, int y);
	virtual void setWindowSize(int width, int height);
	virtual void setWindowResizable(bool resizable);
	virtual void setWindowGhostCorporeal(bool corporeal);
	virtual void setMonitor(int monitor);
	virtual Vector2 getWindowPos();
	virtual Vector2 getWindowSize();
	virtual int getMonitor();
	virtual std::vector<McRect> getMonitors() {return m_vMonitors;}
	virtual Vector2 getNativeScreenSize();
	virtual McRect getVirtualScreenRect();
	virtual McRect getDesktopRect();
	virtual int getDPI();
	virtual bool isFullscreen() {return m_bFullscreen;}
	virtual bool isWindowResizable() {return m_bResizable;}

	// mouse
	virtual bool isCursorInWindow() {return m_bIsCursorInsideWindow;}
	virtual bool isCursorVisible() {return m_bCursorVisible;}
	virtual bool isCursorClipped() {return m_bCursorClipped;}
	virtual Vector2 getMousePos();
	virtual McRect getCursorClip() {return m_cursorClip;}
	virtual CURSORTYPE getCursor() {return m_cursorType;}
	virtual void setCursor(CURSORTYPE cur);
	virtual void setCursorVisible(bool visible);
	virtual void setMousePos(int x, int y);
	virtual void setMousePos(float x, float y);
	virtual void setCursorClip(bool clip, McRect rect);

	// keyboard
	virtual UString keyCodeToString(KEYCODE keyCode);
	virtual void listenToTextInput(bool listen);

	// ILLEGAL:
	void setWindow(SDL_Window *window) {m_window = window;}
	inline SDL_Window *getWindow() {return m_window;}
#ifdef MCENGINE_SDL_TOUCHSUPPORT
	void setWasLastMouseInputTouch(bool wasLastMouseInputTouch) {m_bWasLastMouseInputTouch = wasLastMouseInputTouch;}
	inline bool wasLastMouseInputTouch() const {return m_bWasLastMouseInputTouch;}
#else
	void setWasLastMouseInputTouch(bool unused) {;}
	inline bool wasLastMouseInputTouch() const {return false;}
#endif

	inline void setLastAbsMousePos(Vector2 abs) {m_vLastAbsMousePos=abs;}
	inline void setLastRelMousePos(Vector2 rel) {m_vLastRelMousePos=rel;}

	inline bool sdlDebug() {return m_sdlDebug;}
	inline bool sdlDebug(bool enable) {m_sdlDebug = enable; return m_sdlDebug;}
protected:
	SDL_Window *m_window;

private:
	void onLogLevelChange(UString oldValue, UString newValue);
	ConVar *m_mouse_sensitivity_ref;
	
	bool m_sdlDebug;
	// monitors
	std::vector<McRect> m_vMonitors;

	// window
	bool m_bResizable;
	bool m_bFullscreen;

	// mouse
	bool m_bIsCursorInsideWindow;
	bool m_bCursorVisible;
	bool m_bCursorClipped;
	McRect m_cursorClip;
	CURSORTYPE m_cursorType;

	// the absolute/relative mouse position from the last SDL_PumpEvents call
	// relative only useful if raw input is enabled, value is undefined/garbage otherwise
	Vector2 m_vLastAbsMousePos;
	Vector2 m_vLastRelMousePos;

#ifdef MCENGINE_SDL_TOUCHSUPPORT
	bool m_bWasLastMouseInputTouch;
#else
	static constexpr bool m_bWasLastMouseInputTouch = false;
#endif

	// clipboard
	const char *m_sPrevClipboardTextSDL;
};

#endif

#endif
