//========== Copyright (c) 2018, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		SDL environment functionality
//
// $NoKeywords: $sdlenv
//===============================================================================//

#include "SDLEnvironment.h"

#ifdef MCENGINE_FEATURE_SDL

#include "Mouse.h"

#include "GenericSDLGLESInterface.h"
#include "NullContextMenu.h"
#include "NullGraphicsInterface.h"
#include "SDLGLESInterface.h"
#include "SDLGLLegacyInterface.h"

ConVar debug_sdl("debug_sdl", false, FCVAR_NONE);

[[maybe_unused]] static inline bool isSteamDeckInt()
{
	const char *steamDeck = std::getenv("SteamDeck");
	if (steamDeck != NULL)
	{
		const std::string stdSteamDeck(steamDeck);
		return (stdSteamDeck == "1");
	}
	return false;
}

SDLEnvironment::SDLEnvironment() : Environment()
{
	m_window = nullptr;

	m_bRunning = true;
	m_bDrawing = false;

	m_bMinimized = false; // for fps_max_background
	m_bHasFocus = true;   // for fps_max_background

	m_bHackRestoreRawinput = false; // HACK: set/unset the convar as a global state so we don't mess with the OS cursor when we don't want to

	m_bResizable = false;
	m_bFullscreen = false;

	m_bIsCursorInsideWindow = false;
	m_bCursorVisible = true;
	m_bCursorClipped = false;
	m_cursorType = CURSORTYPE::CURSOR_NORMAL;

#ifdef MCENGINE_SDL_TOUCHSUPPORT
	m_bWasLastMouseInputTouch = false;
#endif

	m_sPrevClipboardTextSDL = NULL;

	// TODO: init monitors
	if (m_vMonitors.size() < 1)
	{
		/// debugLog("WARNING: No monitors found! Adding default monitor ...\n");

		const Vector2 windowSize = getWindowSize();
		m_vMonitors.emplace_back(0, 0, windowSize.x, windowSize.y);
	}

	m_sdlDebug = !!debug_sdl.getInt();
	if (m_sdlDebug)
		onLogLevelChange("", "1");
	debug_sdl.setCallback(fastdelegate::MakeDelegate(this, &SDLEnvironment::onLogLevelChange));

	// TOUCH/JOY LEGACY
	if constexpr (Env::cfg(FEAT::TOUCH))
	{
		m_cvSdl_steamdeck_doubletouch_workaround =
		    new ConVar("sdl_steamdeck_doubletouch_workaround", true, FCVAR_NONE,
		               "currently used to fix a Valve/SDL2 bug on the Steam Deck (fixes \"Touchscreen Native Support\" firing 4 events for one single touch under gamescope, i.e. DOWN/UP/DOWN/UP instead of just DOWN/UP)");
		m_bIsSteamDeck = isSteamDeckInt();
	}
	else
	{
		m_bIsSteamDeck = false;
	}
	if constexpr (Env::cfg(FEAT::JOY | FEAT::JOY_MOU))
	{
		// the switch has its own internal deadzone handling already applied
		m_cvSdl_joystick_mouse_sensitivity = new ConVar("sdl_joystick_mouse_sensitivity", (Env::cfg(OS::HORIZON) ? 0.0f : 1.0f), FCVAR_NONE);
		m_cvSdl_joystick0_deadzone = new ConVar("sdl_joystick0_deadzone", 0.3f, FCVAR_NONE);
		m_cvSdl_joystick_zl_threshold = new ConVar("sdl_joystick_zl_threshold", -0.5f, FCVAR_NONE);
		m_cvSdl_joystick_zr_threshold = new ConVar("sdl_joystick_zr_threshold", -0.5f, FCVAR_NONE);
	}

	m_fJoystick0XPercent = 0.0f;
	m_fJoystick0YPercent = 0.0f;
}

SDLEnvironment::~SDLEnvironment()
{
	SAFE_DELETE(m_engine);
	SAFE_DELETE(m_cvSdl_joystick_mouse_sensitivity);
	SAFE_DELETE(m_cvSdl_joystick0_deadzone);
	SAFE_DELETE(m_cvSdl_joystick_zl_threshold);
	SAFE_DELETE(m_cvSdl_joystick_zr_threshold);
	SAFE_DELETE(m_cvSdl_steamdeck_doubletouch_workaround);
}

void SDLEnvironment::update()
{
	Environment::update();

	m_bIsCursorInsideWindow = McRect(0, 0, m_engine->getScreenWidth(), m_engine->getScreenHeight()).contains(getMousePos());
}

Graphics *SDLEnvironment::createRenderer()
{
#if defined(MCENGINE_FEATURE_GLES2) || defined(MCENGINE_FEATURE_GLES32)

#ifndef __SWITCH__
	return new GenericSDLGLESInterface(m_window);
#else
	return new SDLGLESInterface(m_window); // this just skips glewInit
#endif

#elif defined(MCENGINE_FEATURE_OPENGL)
	return new SDLGLLegacyInterface(m_window);
#else
	return new NullGraphicsInterface();
#endif
}

ContextMenu *SDLEnvironment::createContextMenu()
{
	return new NullContextMenu();
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
	!!us ? SDL_DelayPrecise(us * 1000) : SDL_Delay(0);
}

UString SDLEnvironment::getExecutablePath() const
{
	const char *path = SDL_GetBasePath();
	if (path != NULL)
	{
		UString uPath = UString(path);
		SDL_free((void *)path);
		return uPath;
	}
	else
		return UString("");
}

void SDLEnvironment::openURLInDefaultBrowser(UString url) const
{
	debugLog("WARNING: not available in SDL!\n");
}

UString SDLEnvironment::getUsername() const
{
	return UString("");
}

UString SDLEnvironment::getUserDataPath() const
{
	const char *path = SDL_GetPrefPath("McEngine", "McEngine");
	if (path != NULL)
	{
		UString uPath = UString(path);
		SDL_free((void *)path);
		return uPath;
	}
	else
		return UString("");
}

bool SDLEnvironment::fileExists(UString filename) const
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

bool SDLEnvironment::directoryExists(UString directoryName) const
{
	debugLog("WARNING: not available in SDL!\n");
	return false;
}

bool SDLEnvironment::createDirectory(UString directoryName)
{
	debugLog("WARNING: not available in SDL!\n");
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

std::vector<UString> SDLEnvironment::getFilesInFolder(UString folder) const
{
	debugLog("WARNING: not available in SDL!\n");
	return std::vector<UString>();
}

std::vector<UString> SDLEnvironment::getFoldersInFolder(UString folder) const
{
	debugLog("WARNING: not available in SDL!\n");
	return std::vector<UString>();
}

std::vector<UString> SDLEnvironment::getLogicalDrives() const
{
	debugLog("WARNING: not available in SDL!\n");
	return std::vector<UString>();
}

UString SDLEnvironment::getFolderFromFilePath(UString filepath) const
{
	debugLog("WARNING: not available in SDL!\n");
	return filepath;
}

UString SDLEnvironment::getFileExtensionFromFilePath(UString filepath, bool includeDot) const
{
	const int idx = filepath.findLast(".");
	if (idx != -1)
		return filepath.substr(idx + 1);
	else
		return UString("");
}

UString SDLEnvironment::getFileNameFromFilePath(UString filePath) const
{
	if (filePath.length() < 1)
		return filePath;

	const size_t lastSlashIndex = filePath.findLast("/");
	if (lastSlashIndex != std::string::npos)
		return filePath.substr(lastSlashIndex + 1);

	return filePath;
}

UString SDLEnvironment::getClipBoardText()
{
	if (m_sPrevClipboardTextSDL != NULL)
		SDL_free((void *)m_sPrevClipboardTextSDL);

	m_sPrevClipboardTextSDL = SDL_GetClipboardText();

	return (m_sPrevClipboardTextSDL != NULL ? UString(m_sPrevClipboardTextSDL) : UString(""));
}

void SDLEnvironment::setClipBoardText(UString text)
{
	SDL_SetClipboardText(text.toUtf8());
}

void SDLEnvironment::showMessageInfo(UString title, UString message) const
{
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, title.toUtf8(), message.toUtf8(), m_window);
}

void SDLEnvironment::showMessageWarning(UString title, UString message) const
{
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, title.toUtf8(), message.toUtf8(), m_window);
}

void SDLEnvironment::showMessageError(UString title, UString message) const
{
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title.toUtf8(), message.toUtf8(), m_window);
}

void SDLEnvironment::showMessageErrorFatal(UString title, UString message) const
{
	showMessageError(title, message);
}

UString SDLEnvironment::openFileWindow(const char *filetypefilters, UString title, UString initialpath) const
{
	debugLog("WARNING: not available in SDL!\n");
	return UString("");
}

UString SDLEnvironment::openFolderWindow(UString title, UString initialpath) const
{
	debugLog("WARNING: not available in SDL!\n");
	return UString("");
}

void SDLEnvironment::focus()
{
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
	SDL_MinimizeWindow(m_window);
}

void SDLEnvironment::maximize()
{
	SDL_MaximizeWindow(m_window);
}

void SDLEnvironment::enableFullscreen()
{
	if (m_bFullscreen)
		return;
	if ((m_bFullscreen = SDL_SetWindowFullscreen(m_window, true))) // NOTE: "fake" fullscreen since we don't want a videomode change
		return;
	// if (m_sdlDebug) debugLog("%s %s\n", __PRETTY_FUNCTION__, SDL_GetError());
}

void SDLEnvironment::disableFullscreen()
{
	if (!(m_bFullscreen = !SDL_SetWindowFullscreen(m_window, false)))
		return;
	// if (m_sdlDebug) debugLog("%s %s\n", __PRETTY_FUNCTION__, SDL_GetError());
}

void SDLEnvironment::setWindowTitle(UString title)
{
	SDL_SetWindowTitle(m_window, title.toUtf8());
}

void SDLEnvironment::setWindowPos(int x, int y)
{
	SDL_SetWindowPosition(m_window, x, y);
}

void SDLEnvironment::setWindowSize(int width, int height)
{
	SDL_SetWindowSize(m_window, width, height);
}

void SDLEnvironment::setWindowResizable(bool resizable)
{
	SDL_SetWindowResizable(m_window, resizable);
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

Vector2 SDLEnvironment::getWindowPos() const
{
	int x = 0;
	int y = 0;
	SDL_GetWindowPosition(m_window, &x, &y);
	return Vector2(x, y);
}

Vector2 SDLEnvironment::getWindowSize() const
{
	int width = 100;
	int height = 100;
	SDL_GetWindowSize(m_window, &width, &height);
	return Vector2(width, height);
}

int SDLEnvironment::getMonitor() const
{
	const int display = static_cast<int>(SDL_GetDisplayForWindow(m_window)); // HACK: 0 means invalid display in SDL, decrement by 1 for engine/app
	return display - 1 < 0 ? 0 : display - 1;
}

Vector2 SDLEnvironment::getNativeScreenSize() const
{
	SDL_DisplayID di = SDL_GetDisplayForWindow(m_window);
	return Vector2(SDL_GetDesktopDisplayMode(di)->w, SDL_GetDesktopDisplayMode(di)->h);
}

McRect SDLEnvironment::getVirtualScreenRect() const
{
	// TODO:
	return McRect(0, 0, 1, 1);
}

McRect SDLEnvironment::getDesktopRect() const
{
	Vector2 screen = getNativeScreenSize();
	return McRect(0, 0, screen.x, screen.y);
}

int SDLEnvironment::getDPI() const
{
	float dpi = SDL_GetWindowDisplayScale(m_window) * 96;

	return clamp<int>((int)dpi, 96, 96 * 2); // sanity clamp
}

Vector2 SDLEnvironment::getMousePos() const
{
	// HACKHACK: workaround, we don't want any finger besides the first initial finger changing the position
	// NOTE: on the Steam Deck, even with "Touchscreen Native Support" enabled, SDL_GetMouseState() will always return the most recent touch position, which we do not want
	if constexpr (Env::cfg(FEAT::TOUCH))
		if (m_bWasLastMouseInputTouch)
			return m_engine->getMouse()->getActualPos(); // so instead, we return our own which is always guaranteed to be the first finger position (and no other fingers)

	return m_vLastAbsMousePos;
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

void SDLEnvironment::setCursorClip(bool clip, McRect rect)
{
	m_cursorClip = rect;

	if (clip)
	{
		const SDL_Rect clipRect{static_cast<int>(rect.getX()), static_cast<int>(rect.getY()), static_cast<int>(rect.getWidth()), static_cast<int>(rect.getHeight())};
		SDL_SetWindowMouseRect(m_window, &clipRect);
		SDL_SetWindowMouseGrab(m_window, true);
		m_bCursorClipped = true;
	}
	else
	{
		m_bCursorClipped = false;
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

void SDLEnvironment::listenToTextInput(bool listen)
{
	listen ? SDL_StartTextInput(m_window) : SDL_StopTextInput(m_window);
	SDL_SetWindowKeyboardGrab(m_window, !listen);
}

void SDLEnvironment::onLogLevelChange(UString oldValue, UString newValue)
{
	const bool enable = !!newValue.toInt();
	if (enable)
	{
		sdlDebug(true);
		SDL_SetLogPriorities(SDL_LOG_PRIORITY_TRACE);
	}
	else
	{
		sdlDebug(false);
		SDL_ResetLogPriorities();
	}
}

#endif
