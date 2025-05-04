//========== Copyright (c) 2018, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		SDL environment functionality
//
// $NoKeywords: $sdlenv
//===============================================================================//

#include "SDLEnvironment.h"

#include <algorithm>

#ifdef MCENGINE_FEATURE_SDL

#include "Mouse.h"

#include "NullContextMenu.h"
#include "NullGraphicsInterface.h"
#include "SDLGLInterface.h"

#if defined(MCENGINE_PLATFORM_WINDOWS)
#include <lmcons.h>
#include <windows.h>
#elif defined(__APPLE__) || defined(MCENGINE_PLATFORM_LINUX)
#include <pwd.h>
#include <unistd.h>
#elif defined(__SWITCH__)
#include <dirent.h>
#include <switch.h>
#include <sys/stat.h>
#elif defined(__EMSCRIPTEN__)
// TODO
#endif

// definitions
ConVar debug_sdl("debug_sdl", false, FCVAR_NONE);
SDLEnvironment::FileDialogState SDLEnvironment::s_fileDialogState = {.done = true, .result = ""};

// declarations
static inline std::vector<UString> enumerateDirectory(const char *pathToEnum, SDL_PathType type);
static inline void winSortInPlace(std::vector<UString> &toSort);
[[maybe_unused]] static inline bool isSteamDeckInt();

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

	// create sdl system cursor map
	m_mCursorIcons = {
	    {CURSORTYPE::CURSOR_NORMAL, SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT)},      {CURSORTYPE::CURSOR_WAIT, SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT)},
	    {CURSORTYPE::CURSOR_SIZE_H, SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_EW_RESIZE)},    {CURSORTYPE::CURSOR_SIZE_V, SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NS_RESIZE)},
	    {CURSORTYPE::CURSOR_SIZE_HV, SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NESW_RESIZE)}, {CURSORTYPE::CURSOR_SIZE_VH, SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NWSE_RESIZE)},
	    {CURSORTYPE::CURSOR_TEXT, SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_TEXT)},
	};

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
		               "currently used to fix a Valve/SDL2 bug on the Steam Deck (fixes \"Touchscreen Native Support\" firing 4 events for one single touch "
		               "under gamescope, i.e. DOWN/UP/DOWN/UP instead of just DOWN/UP)");
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
	for (auto cur : m_mCursorIcons)
	{
		SDL_DestroyCursor(cur.second);
	}
}

void SDLEnvironment::update()
{
	Environment::update();

	m_bIsCursorInsideWindow = McRect(0, 0, m_engine->getScreenWidth(), m_engine->getScreenHeight()).contains(getMousePos());
}

Graphics *SDLEnvironment::createRenderer()
{
#if defined(MCENGINE_FEATURE_GLES2) || defined(MCENGINE_FEATURE_GLES32) || defined(MCENGINE_FEATURE_OPENGL)
	return new SDLGLInterface(m_window);
#else // TODO hook up DX11/OpenGL3
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
	// TODO: (maybe restart, might never be necessary)
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
	if (!SDL_OpenURL(url.toUtf8()))
		debugLog("Failed to open URL: %s\n", SDL_GetError());
}

UString SDLEnvironment::getUsername() const
{
#if defined(MCENGINE_PLATFORM_WINDOWS)
	DWORD username_len = UNLEN + 1;
	wchar_t username[UNLEN + 1];

	if (GetUserNameW(username, &username_len))
		return {username};
#elif defined(__APPLE__) || defined(MCENGINE_PLATFORM_LINUX)
	const char *user = getenv("USER");
	if (user != nullptr)
		return {user};

	struct passwd *pwd = getpwuid(getuid());
	if (pwd != nullptr)
		return {pwd->pw_name};
#elif defined(__SWITCH__) // copied from HorizonSDLEnvironment
	UString uUsername = convar->getConVarByName("name")->getString();

	// this was directly taken from the libnx examples

	Result rc = 0;

	AccountUid userID;
	/// bool account_selected = 0;
	AccountProfile profile;
	AccountUserData userdata;
	AccountProfileBase profilebase;

	char username[0x21];

	memset(&userdata, 0, sizeof(userdata));
	memset(&profilebase, 0, sizeof(profilebase));

	rc = accountInitialize(AccountServiceType_Application);
	if (R_FAILED(rc))
		debugLog("accountInitialize() failed: 0x%x\n", rc);

	if (R_SUCCEEDED(rc))
	{
		rc = accountGetLastOpenedUser(&userID);

		if (R_FAILED(rc))
			debugLog("accountGetActiveUser() failed: 0x%x\n", rc);

		if (R_SUCCEEDED(rc))
		{

			rc = accountGetProfile(&profile, userID);

			if (R_FAILED(rc))
				debugLog("accountGetProfile() failed: 0x%x\n", rc);
		}

		if (R_SUCCEEDED(rc))
		{
			rc = accountProfileGet(&profile, &userdata, &profilebase); // userdata is otional, see libnx acc.h.

			if (R_FAILED(rc))
				debugLog("accountProfileGet() failed: 0x%x\n", rc);

			if (R_SUCCEEDED(rc))
			{
				memset(username, 0, sizeof(username));
				strncpy(username, profilebase.nickname, sizeof(username) - 1); // even though profilebase.username usually has a NUL-terminator, don't assume it does for safety.
				debugLog("Username: %s\n", username);
				uUsername = UString(username);
			}
			accountProfileClose(&profile);
		}
		accountExit();
	}

	return uUsername;
#endif
	// fallback
	return {PACKAGE_NAME "user"};
}

UString SDLEnvironment::getUserDataPath() const
{
	const char *path = SDL_GetPrefPath("McEngine", PACKAGE_NAME);
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
	SDL_PathInfo info;
	if (SDL_GetPathInfo(directoryName.toUtf8(), &info))
		return info.type == SDL_PATHTYPE_DIRECTORY;

	return false;
}

bool SDLEnvironment::createDirectory(UString directoryName) const
{
	return SDL_CreateDirectory(directoryName.toUtf8());
}

bool SDLEnvironment::renameFile(UString oldFileName, UString newFileName)
{
	return std::rename(oldFileName.toUtf8(), newFileName.toUtf8()); // TODO: use SDL for this?
}

bool SDLEnvironment::deleteFile(UString filePath)
{
	return std::remove(filePath.toUtf8()) == 0; // TODO: maybe use SDL for this?
}

std::vector<UString> SDLEnvironment::getFilesInFolder(UString folder) const
{
	return enumerateDirectory(folder.toUtf8(), SDL_PATHTYPE_FILE);
}

std::vector<UString> SDLEnvironment::getFoldersInFolder(UString folder) const
{
	// TODO: if this turns out to be too slow for folders with a lot of subfolders, split out the sorting
	// currently only the skinlist really uses it, shouldn't have more than 5000 skins in it for normal human beings
	auto folders = enumerateDirectory(folder.toUtf8(), SDL_PATHTYPE_DIRECTORY);
	winSortInPlace(folders);
	return folders;
}

// sadly, sdl doesn't give a way to do this
std::vector<UString> SDLEnvironment::getLogicalDrives() const
{
	std::vector<UString> drives{};

	if constexpr (Env::cfg(OS::HORIZON))
	{
		drives.emplace_back("sdmc");
		drives.emplace_back("romfs");
	}
	else if constexpr (Env::cfg(OS::LINUX | OS::MACOS))
	{
		drives.emplace_back("/");
	}
	else if constexpr (Env::cfg(OS::WINDOWS))
	{
#if defined(MCENGINE_PLATFORM_WINDOWS)
		DWORD dwDrives = GetLogicalDrives();
		for (int i = 0; i < 26; i++) // A-Z
		{
			if (dwDrives & (1 << i))
			{
				char driveLetter = 'A' + i;
				UString drivePath = UString::format("%c:/", driveLetter);

				SDL_PathInfo info;
				UString testPath = UString::format("%c:\\", driveLetter);

				if (SDL_GetPathInfo(testPath.toUtf8(), &info))
				{
					drives.emplace_back(drivePath);
				}
			}
		}
#endif
	}
	else if constexpr (Env::cfg(OS::WASM))
	{
		// TODO: VFS
		drives.emplace_back("/");
	}

	return drives;
}

UString SDLEnvironment::getFolderFromFilePath(UString filepath) const
{
	if (filepath.length() < 1)
		return filepath;

	size_t lastSlash = filepath.findLast("/");
	if (lastSlash != std::string::npos)
		return filepath.substr(0, lastSlash + 1);

	lastSlash = filepath.findLast("\\");
	if (lastSlash != std::string::npos)
		return filepath.substr(0, lastSlash + 1);

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

UString SDLEnvironment::getFileNameFromFilePath(UString filepath) const
{
	if (filepath.length() < 1)
		return filepath;

	const size_t lastSlashIndex = filepath.findLast("/");
	if (lastSlashIndex != std::string::npos)
		return filepath.substr(lastSlashIndex + 1);

	return filepath;
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

void SDLEnvironment::fileDialogCallback(void *userdata, const char *const *filelist, int filter)
{
	if (filelist && *filelist)
	{
		s_fileDialogState.result = UString(*filelist);
	}
	else
	{
		s_fileDialogState.result = "";
	}
	s_fileDialogState.done = true;
}

UString SDLEnvironment::openFileWindow(const char *filetypefilters, UString title, UString initialpath) const
{
	// convert filetypefilters (Windows-style)
	std::vector<std::string> filterNames;
	std::vector<std::string> filterPatterns;
	std::vector<SDL_DialogFileFilter> sdlFilters;

	if (filetypefilters && *filetypefilters)
	{
		const char *curr = filetypefilters;
		// add the filetype filters to the SDL dialog filter
		while (*curr)
		{
			const char *name = curr;
			curr += strlen(name) + 1;

			if (!*curr)
				break;

			const char *pattern = curr;
			curr += strlen(pattern) + 1;

			filterNames.emplace_back(name);
			filterPatterns.emplace_back(pattern);

			SDL_DialogFileFilter filter = {filterNames.back().c_str(), filterPatterns.back().c_str()};
			sdlFilters.push_back(filter);
		}
	}

	// reset static file dialog state
	s_fileDialogState.done = false;
	s_fileDialogState.result = "";

	// show it
	SDL_ShowOpenFileDialog(fileDialogCallback, nullptr, m_window, sdlFilters.empty() ? nullptr : sdlFilters.data(), static_cast<int>(sdlFilters.size()),
	                       initialpath.length() > 0 ? initialpath.toUtf8() : nullptr, false);

	// wait for it to close
	while (!s_fileDialogState.done)
	{
		SDL_Event event;
		if (SDL_WaitEventTimeout(&event, 100))
			if (event.type == SDL_EVENT_QUIT)
				break;
	}

	return s_fileDialogState.result;
}

// TODO: what if multiple of these could be opened at once? or openFolderWindow + openFileWindow?
UString SDLEnvironment::openFolderWindow(UString title, UString initialpath) const
{
	// reset static file dialog state
	s_fileDialogState.done = false;
	s_fileDialogState.result = "";

	// show it
	SDL_ShowOpenFolderDialog(fileDialogCallback, nullptr, m_window, initialpath.length() > 0 ? initialpath.toUtf8() : nullptr, false);

	// wait for it to close
	while (!s_fileDialogState.done)
	{
		SDL_Event event;
		if (SDL_WaitEventTimeout(&event, 100))
			if (event.type == SDL_EVENT_QUIT)
				break;
	}

	return s_fileDialogState.result;
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
	// TODO (wtf is this?)
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
	// NOTE: on the Steam Deck, even with "Touchscreen Native Support" enabled, SDL_GetMouseState() will always return the most recent touch position, which we
	// do not want
	if constexpr (Env::cfg(FEAT::TOUCH))
		if (m_bWasLastMouseInputTouch)
			return m_engine->getMouse()->getActualPos(); // so instead, we return our own which is always guaranteed to be the first finger position (and no other fingers)

	return m_vLastAbsMousePos;
}

void SDLEnvironment::setCursor(CURSORTYPE cur)
{
	m_cursorType = cur;

	SDL_SetCursor(m_mCursorIcons.at(m_cursorType)); // does not make visible if the cursor isn't visible
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

// static helpers

// for getting files in folder/ folders in folder
static inline std::vector<UString> enumerateDirectory(const char *pathToEnum, SDL_PathType type)
{
	struct EnumContext
	{
		UString dirName;
		std::vector<UString> *contents;
		SDL_PathType type;
	};

	auto enumCallback = [](void *userData, const char *, const char *fname) -> SDL_EnumerationResult {
		auto *ctx = static_cast<EnumContext *>(userData);

		if (std::strcmp(fname, ".") == 0 || std::strcmp(fname, "..") == 0)
			return SDL_ENUM_CONTINUE;

		SDL_PathInfo info;
		if (SDL_GetPathInfo((ctx->dirName + fname).toUtf8(), &info) && info.type == ctx->type)
			ctx->contents->emplace_back(fname); // only want the filename

		return SDL_ENUM_CONTINUE;
	};

	UString path = pathToEnum;
	if (!path.endsWith('/') && !path.endsWith('\\'))
		path += '/';

	std::vector<UString> contents;

	EnumContext context{.dirName = path, .contents = &contents, .type = type};

	if (!SDL_EnumerateDirectory(path.toUtf8(), enumCallback, &context))
		debugLog("Failed to enumerate directory: %s\n", SDL_GetError());

	return contents;
}

// return a more naturally windows-like sorted order for folders, useful for e.g. osu! skin list dropdown order
static inline void winSortInPlace(std::vector<UString> &toSort)
{
	auto naturalCompare = [](const UString &a, const UString &b) -> bool {
		const char *aStr = a.toUtf8();
		const char *bStr = b.toUtf8();

		while (*aStr && *bStr) // skip to the first difference
		{
			bool aIsDigit = std::isdigit(static_cast<unsigned char>(*aStr));
			bool bIsDigit = std::isdigit(static_cast<unsigned char>(*bStr));

			if (aIsDigit != bIsDigit) // different types
				return aIsDigit;

			if (aIsDigit) // collect and compare the complete numbers if both are digits
			{
				const char *aNumStart = aStr;
				const char *bNumStart = bStr;
				while (*aStr == '0' && std::isdigit(static_cast<unsigned char>(*(aStr + 1)))) // skip leading zeros
					++aStr;
				while (*bStr == '0' && std::isdigit(static_cast<unsigned char>(*(bStr + 1))))
					++bStr;
				const char *aPtr = aStr;
				const char *bPtr = bStr;
				while (std::isdigit(static_cast<unsigned char>(*aPtr))) // count digits
					++aPtr;
				while (std::isdigit(static_cast<unsigned char>(*bPtr)))
					++bPtr;
				size_t aDigits = aPtr - aStr;
				size_t bDigits = bPtr - bStr;
				if (aDigits != bDigits) // different digit counts
					return aDigits < bDigits;
				while (aStr < aPtr) // same number of digits, compare them
				{
					if (*aStr != *bStr)
						return *aStr < *bStr;
					++aStr;
					++bStr;
				}
				// numbers are equal, check if the originals had different lengths due to leading zeros
				size_t aFullLen = aPtr - aNumStart;
				size_t bFullLen = bPtr - bNumStart;
				if (aFullLen != bFullLen)
					return aFullLen < bFullLen;
			}
			else
			{
				// simple case-insensitive compare if neither are digits
				char aLower = std::tolower(static_cast<unsigned char>(*aStr));
				char bLower = std::tolower(static_cast<unsigned char>(*bStr));
				if (aLower != bLower)
					return aLower < bLower;
				++aStr;
				++bStr;
			}
		}
		return *aStr == 0 && *bStr != 0;
	};
	std::ranges::sort(toSort, naturalCompare);
}

// for touch support hack (DEPRECATED)
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

#endif
