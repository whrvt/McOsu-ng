//================ Copyright (c) 2018, PG, All rights reserved. =================//
//
// Purpose:		SDL ("partial", SDL does not provide all functions!)
//
// $NoKeywords: $sdlenv
//===============================================================================//

#include "SDLEnvironment.h"

#ifdef MCENGINE_FEATURE_SDL

#include "Engine.h"
#include "Mouse.h"
#include "ConVar.h"

#include "NullGraphicsInterface.h"
#include "SDLGLLegacyInterface.h"
#include "SDLGLES2Interface.h"
#include "WinSDLGLES2Interface.h"
#include "NullContextMenu.h"

SDLEnvironment::SDLEnvironment(SDL_Window *window) : Environment()
{
	m_window = window;

	m_mouse_sensitivity_ref = convar->getConVarByName("mouse_sensitivity");

	m_bResizable = true;
	m_bFullscreen = false;

	m_bIsCursorInsideWindow = false;
	m_bCursorVisible = true;
	m_bCursorClipped = false;
	m_cursorType = CURSORTYPE::CURSOR_NORMAL;

	m_bWasLastMouseInputTouch = false;

	m_sPrevClipboardTextSDL = NULL;

	// TODO: init monitors
	if (m_vMonitors.size() < 1)
	{
		///debugLog("WARNING: No monitors found! Adding default monitor ...\n");

		const Vector2 windowSize = getWindowSize();
		m_vMonitors.push_back(McRect(0, 0, windowSize.x, windowSize.y));
	}
}

void SDLEnvironment::update()
{
	Environment::update();

	m_bIsCursorInsideWindow = McRect(0, 0, engine->getScreenWidth(), engine->getScreenHeight()).contains(getMousePos());
}

Graphics *SDLEnvironment::createRenderer()
{
	//return new NullGraphicsInterface();
	return new SDLGLLegacyInterface(m_window);

#ifdef MCENGINE_FEATURE_OPENGLES

#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__CYGWIN__) || defined(__CYGWIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__)

	return new WinSDLGLES2Interface(m_window);

#else

	return new SDLGLES2Interface(m_window);

#endif

#endif
}

ContextMenu *SDLEnvironment::createContextMenu()
{
	return new NullContextMenu();
}

Environment::OS SDLEnvironment::getOS()
{
	return Environment::OS::OS_NULL;
}

void SDLEnvironment::shutdown()
{
	SDL_Event event;
	event.type = SDL_EVENT_QUIT;
	SDL_PushEvent(&event);
}

void SDLEnvironment::restart()
{
	// TODO:
	shutdown();
}

void SDLEnvironment::sleep(unsigned int us)
{
	!!us ? SDL_DelayPrecise(us*1000) : SDL_Delay(0);
}

UString SDLEnvironment::getExecutablePath()
{
	const char *path = SDL_GetBasePath();
	if (path != NULL)
	{
		UString uPath = UString(path);
		SDL_free((void*)path);
		return uPath;
	}
	else
		return UString("");
}

void SDLEnvironment::openURLInDefaultBrowser(UString url)
{
	debugLog("WARNING: SDLEnvironment::openURLInDefaultBrowser() not available in SDL!\n");
}

UString SDLEnvironment::getUsername()
{
	return UString("");
}

UString SDLEnvironment::getUserDataPath()
{
	const char *path = SDL_GetPrefPath("McEngine", "McEngine");
	if (path != NULL)
	{
		UString uPath = UString(path);
		SDL_free((void*)path);
		return uPath;
	}
	else
		return UString("");
}

bool SDLEnvironment::fileExists(UString filename)
{
	SDL_IOStream *file = SDL_IOFromFile(filename.toUtf8(), "r");
	if (file != NULL)
	{
		SDL_CloseIO(file);
		return true;
	}
	else
		return false;
}

bool SDLEnvironment::directoryExists(UString directoryName)
{
	debugLog("WARNING: SDLEnvironment::directoryExists() not available in SDL!\n");
	return false;
}

bool SDLEnvironment::createDirectory(UString directoryName)
{
	debugLog("WARNING: SDLEnvironment::createDirectory() not available in SDL!\n");
	return false;
}

bool SDLEnvironment::renameFile(UString oldFileName, UString newFileName)
{
	return std::rename(oldFileName.toUtf8(), newFileName.toUtf8());
}

bool SDLEnvironment::deleteFile(UString filePath)
{
	return std::remove(filePath.toUtf8()) == 0;
}

std::vector<UString> SDLEnvironment::getFilesInFolder(UString folder)
{
	debugLog("WARNING: SDLEnvironment::getFilesInFolder() not available in SDL!\n");
	return std::vector<UString>();
}

std::vector<UString> SDLEnvironment::getFoldersInFolder(UString folder)
{
	debugLog("WARNING: SDLEnvironment::getFoldersInFolder() not available in SDL!\n");
	return std::vector<UString>();
}

std::vector<UString> SDLEnvironment::getLogicalDrives()
{
	debugLog("WARNING: SDLEnvironment::getLogicalDrives() not available in SDL!\n");
	return std::vector<UString>();
}

UString SDLEnvironment::getFolderFromFilePath(UString filepath)
{
	debugLog("WARNING: SDLEnvironment::getFolderFromFilePath() not available in SDL!\n");
	return filepath;
}

UString SDLEnvironment::getFileExtensionFromFilePath(UString filepath, bool includeDot)
{
	const int idx = filepath.findLast(".");
	if (idx != -1)
		return filepath.substr(idx+1);
	else
		return UString("");
}

UString SDLEnvironment::getFileNameFromFilePath(UString filePath)
{
	if (filePath.length() < 1) return filePath;

	const size_t lastSlashIndex = filePath.findLast("/");
	if (lastSlashIndex != std::string::npos)
		return filePath.substr(lastSlashIndex + 1);

	return filePath;
}

UString SDLEnvironment::getClipBoardText()
{
	if (m_sPrevClipboardTextSDL != NULL)
		SDL_free((void*)m_sPrevClipboardTextSDL);

	m_sPrevClipboardTextSDL = SDL_GetClipboardText();

	return (m_sPrevClipboardTextSDL != NULL ? UString(m_sPrevClipboardTextSDL) : UString(""));
}

void SDLEnvironment::setClipBoardText(UString text)
{
	SDL_SetClipboardText(text.toUtf8());
}

void SDLEnvironment::showMessageInfo(UString title, UString message)
{
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, title.toUtf8(), message.toUtf8(), m_window);
}

void SDLEnvironment::showMessageWarning(UString title, UString message)
{
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, title.toUtf8(), message.toUtf8(), m_window);
}

void SDLEnvironment::showMessageError(UString title, UString message)
{
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title.toUtf8(), message.toUtf8(), m_window);
}

void SDLEnvironment::showMessageErrorFatal(UString title, UString message)
{
	showMessageError(title, message);
}

UString SDLEnvironment::openFileWindow(const char *filetypefilters, UString title, UString initialpath)
{
	debugLog("WARNING: SDLEnvironment::openFileWindow() not available in SDL!\n");
	return UString("");
}

UString SDLEnvironment::openFolderWindow(UString title, UString initialpath)
{
	debugLog("WARNING: SDLEnvironment::openFolderWindow() not available in SDL!\n");
	return UString("");
}

void SDLEnvironment::focus()
{
    SDL_SyncWindow(m_window);
	SDL_RaiseWindow(m_window);
}

void SDLEnvironment::center()
{
    SDL_SyncWindow(m_window);
    const SDL_DisplayID di = SDL_GetDisplayForWindow(m_window);
	SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED_DISPLAY(di), SDL_WINDOWPOS_CENTERED_DISPLAY(di));
}

void SDLEnvironment::minimize()
{
    SDL_SyncWindow(m_window);
	SDL_MinimizeWindow(m_window);
}

void SDLEnvironment::maximize()
{
    SDL_SyncWindow(m_window);
	SDL_MaximizeWindow(m_window);
}

void SDLEnvironment::enableFullscreen()
{
	if ((m_bFullscreen = !!SDL_GetWindowFullscreenMode(m_window))) return;
	if ((m_bFullscreen = SDL_SetWindowFullscreen(m_window, true))) return; // NOTE: "fake" fullscreen since we don't want a videomode change
	if (m_sdlDebug) debugLog("%s %s\n", __PRETTY_FUNCTION__, SDL_GetError());
}

void SDLEnvironment::disableFullscreen()
{
	if ((m_bFullscreen = !SDL_GetWindowFullscreenMode(m_window))) return;
	if ((m_bFullscreen = SDL_SetWindowFullscreen(m_window, false))) return;
	if (m_sdlDebug) debugLog("%s %s\n", __PRETTY_FUNCTION__, SDL_GetError());
}

void SDLEnvironment::setWindowTitle(UString title)
{
	SDL_SetWindowTitle(m_window, title.toUtf8());
}

void SDLEnvironment::setWindowPos(int x, int y)
{
    SDL_SyncWindow(m_window);
	SDL_SetWindowPosition(m_window, x, y);
}

void SDLEnvironment::setWindowSize(int width, int height)
{
    SDL_SyncWindow(m_window);
	SDL_SetWindowSize(m_window, width, height);
}

void SDLEnvironment::setWindowResizable(bool resizable)
{
	SDL_SetWindowResizable(m_window, resizable ? true : false);
	m_bResizable = resizable;
}

void SDLEnvironment::setWindowGhostCorporeal(bool corporeal)
{
	// TODO
}

void SDLEnvironment::setMonitor(int monitor)
{
	// TODO:
	center();
}

Vector2 SDLEnvironment::getWindowPos()
{
	int x = 0;
	int y = 0;
	{
        SDL_SyncWindow(m_window);
		SDL_GetWindowPosition(m_window, &x, &y);
	}
	return Vector2(x, y);
}

Vector2 SDLEnvironment::getWindowSize()
{
	int width = 100;
	int height = 100;
	{
        SDL_SyncWindow(m_window);
		SDL_GetWindowSize(m_window, &width, &height);
	}
	return Vector2(width, height);
}

int SDLEnvironment::getMonitor()
{
	// TODO: needs to be implemented in combination with center() etc.
	return 0;
	//const int monitor = SDL_GetWindowDisplayIndex(m_window);
	//return (monitor != -1 ? monitor : 0);
}

Vector2 SDLEnvironment::getNativeScreenSize()
{
    SDL_SyncWindow(m_window);
	SDL_DisplayID di = SDL_GetDisplayForWindow(m_window);
	return Vector2(SDL_GetDesktopDisplayMode(di)->w, SDL_GetDesktopDisplayMode(di)->h);
}

McRect SDLEnvironment::getVirtualScreenRect()
{
	// TODO:
	return McRect(0, 0, 1, 1);
}

McRect SDLEnvironment::getDesktopRect()
{
	Vector2 screen = getNativeScreenSize();
	return McRect(0, 0, screen.x, screen.y);
}

int SDLEnvironment::getDPI()
{
	float dpi = SDL_GetWindowDisplayScale(m_window) * 96;

	return clamp<int>((int)dpi, 96, 96*2); // sanity clamp
}

Vector2 SDLEnvironment::getMousePos()
{
	// HACKHACK: workaround, we don't want any finger besides the first initial finger changing the position
	// NOTE: on the Steam Deck, even with "Touchscreen Native Support" enabled, SDL_GetMouseState() will always return the most recent touch position, which we do not want
	if (m_bWasLastMouseInputTouch)
		return engine->getMouse()->getActualPos(); // so instead, we return our own which is always guaranteed to be the first finger position (and no other fingers)

	float mouseX = 0;
	float mouseY = 0;

	// HACKHACK: workaround, don't change this
	if (m_mouse_sensitivity_ref->getFloat() == 1.0f)
	{
		SDL_GetMouseState(&mouseX, &mouseY);
		//debugLog("x %f y %f\n", mouseX, mouseY);
		return Vector2(mouseX, mouseY);
	}
	else
	{

		int windowX = 0;
		int windowY = 0;

		SDL_GetGlobalMouseState(&mouseX, &mouseY);
		SDL_GetWindowPosition(m_window, &windowX, &windowY);

		return Vector2(mouseX - static_cast<float>(windowX), mouseY - static_cast<float>(windowY));
	}
}

void SDLEnvironment::setCursor(CURSORTYPE cur)
{
	// (not properly supported in SDL)
}

void SDLEnvironment::setCursorVisible(bool visible)
{
	visible ? SDL_ShowCursor() : SDL_HideCursor();

	m_bCursorVisible = visible;
}

void SDLEnvironment::setMousePos(int x, int y)
{
	SDL_WarpMouseInWindow(m_window, (float)x, (float)y);
}

void SDLEnvironment::setMousePos(float x, float y)
{
	SDL_WarpMouseInWindow(m_window, x, y);
}

void SDLEnvironment::setCursorClip(bool clip, McRect rect)
{
	m_cursorClip = rect;

	if (clip)
	{
		const SDL_Rect clipRect {
			static_cast<int>(rect.getX()),
			static_cast<int>(rect.getY()),
			static_cast<int>(rect.getWidth()),
			static_cast<int>(rect.getHeight())
		};
        m_bCursorClipped = true;
		SDL_SetWindowMouseRect(m_window, &clipRect);
		SDL_SetWindowMouseGrab(m_window, true);
		// HACK: sdl2->sdl3 don't listen for text input when in play mode (putting it here for now to reuse "in-gameplay" logic)
		SDL_StopTextInput(m_window);
	}
	else
	{
        m_bCursorClipped = false;
        SDL_StartTextInput(m_window);
        SDL_SetWindowMouseGrab(m_window, false);
		SDL_SetWindowMouseRect(m_window, NULL);
	}
}

UString SDLEnvironment::keyCodeToString(KEYCODE keyCode)
{
	const char *name = SDL_GetScancodeName((SDL_Scancode)keyCode);
	if (name == NULL)
		return UString::format("%lu", keyCode);
	else
	{
		UString uName = UString(name);
		if (uName.length() < 1)
			return UString::format("%lu", keyCode);
		else
			return uName;
	}
}

#endif
