//========== Copyright (c) 2018, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		top level interface for native OS calls
//
// $NoKeywords: $env
//===============================================================================//

#include "Environment.h"

#include "Engine.h"
#include "Mouse.h"

#include "NullContextMenu.h"
#include "SDLGLInterface.h"

#if defined(MCENGINE_PLATFORM_WINDOWS)
#include <lmcons.h>
#include <windows.h>
#elif defined(__APPLE__) || defined(MCENGINE_PLATFORM_LINUX)
#include <pwd.h>
#include <unistd.h>
#elif defined(__EMSCRIPTEN__)
// TODO
#endif

#include <algorithm>
#include <cassert>
#include <cstddef>

// extern convars
extern ConVar mouse_sensitivity;
extern ConVar mouse_raw_input;

// declarations
static inline std::vector<UString> enumerateDirectory(const char *pathToEnum, SDL_PathType type);
static inline void winSortInPlace(std::vector<UString> &toSort);
static void sensTransformFunc(void *userdata, Uint64 timestamp, SDL_Window *window, SDL_MouseID mouseID, float *x, float *y);

// definitions
ConVar debug_sdl("debug_sdl", false, FCVAR_NONE);
ConVar _debug_env("debug_env", false, FCVAR_NONE);
ConVar _fullscreen_windowed_borderless_("fullscreen_windowed_borderless", false, FCVAR_NONE);
ConVar _monitor_("monitor", 0, FCVAR_NONE, "monitor/display device to switch to, 0 = primary monitor");
ConVar *Environment::debug_env = &_debug_env;
Environment::FileDialogState Environment::s_fileDialogState = {.done = true, .result = ""};

Environment::Environment()
{
	m_window = nullptr;

	m_bRunning = true;
	m_bDrawing = false;

	m_bMinimized = false; // for fps_max_background
	m_bHasFocus = true;   // for fps_max_background
	m_bFullscreenWindowedBorderless = false;

	m_bIsRawInput = false;
	m_bCursorVisible = false;

	m_bResizable = false;
	m_bFullscreen = false;

	m_sUsername = {};
	m_sProgDataPath = {}; // local data for McEngine files
	m_sAppDataPath = {};
	m_hwnd = nullptr;

	m_bIsCursorInsideWindow = false;
	m_bCursorVisible = true;
	m_bCursorClipped = false;
	m_cursorType = CURSORTYPE::CURSOR_NORMAL;

	m_vLastAbsMousePos = {};
	m_vLastRelMousePos = {};

	m_sFpsMax = "60";
	m_sFpsMaxBG = "30";

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

	// setup callbacks
	m_sdlDebug = !!debug_sdl.getInt();
	if (m_sdlDebug)
		onLogLevelChange(1.0f);
	debug_sdl.setCallback(fastdelegate::MakeDelegate(this, &Environment::onLogLevelChange));
	mouse_raw_input.setCallback(fastdelegate::MakeDelegate(this, &Environment::onRawInputChange));
	_fullscreen_windowed_borderless_.setCallback(fastdelegate::MakeDelegate(this, &Environment::onFullscreenWindowBorderlessChange));
	_monitor_.setCallback(fastdelegate::MakeDelegate(this, &Environment::onMonitorChange));
}

Environment::~Environment()
{
	SAFE_DELETE(m_engine);
	for (auto cur : m_mCursorIcons)
	{
		SDL_DestroyCursor(cur.second);
	}
}

// well this doesn't do much atm... called at the end of engine->onUpdate
void Environment::update()
{
	//Environment::update();

	m_bIsCursorInsideWindow = m_bHasFocus && McRect(0, 0, m_engine->getScreenWidth(), m_engine->getScreenHeight()).contains(getMousePos());
}

Graphics *Environment::createRenderer()
{
#if defined(MCENGINE_FEATURE_GLES2) || defined(MCENGINE_FEATURE_GLES32) || defined(MCENGINE_FEATURE_OPENGL)
	return new SDLGLInterface(m_window);
#else // TODO hook up DX11/OpenGL3
	return new NullGraphicsInterface();
#endif
}

ContextMenu *Environment::createContextMenu() // DEPRECATED
{
	return new NullContextMenu();
}

void Environment::shutdown()
{
	SDL_Event event;
	event.type = SDL_EVENT_QUIT;
	SDL_PushEvent(&event);
}

void Environment::restart()
{
	// TODO (probably the environment should never be restarted, just the engine (somehow))
	shutdown();
}

UString Environment::getExecutablePath() const
{
	const char *path = SDL_GetBasePath();
	if (!path)
		return {Env::cfg(OS::WINDOWS) ? ".\\" : "./"};

	return {path};
}

void Environment::openURLInDefaultBrowser(UString url) const
{
	if (!SDL_OpenURL(url.toUtf8()))
		debugLog("Failed to open URL: %s\n", SDL_GetError());
}

UString Environment::getUsername()
{
	if (!m_sUsername.isEmpty())
		return m_sUsername;
#if defined(MCENGINE_PLATFORM_WINDOWS)
	DWORD username_len = UNLEN + 1;
	wchar_t username[UNLEN + 1];

	if (GetUserNameW(username, &username_len))
		m_sUsername = {username};
#elif defined(__APPLE__) || defined(MCENGINE_PLATFORM_LINUX) || defined(MCENGINE_PLATFORM_WASM)
	const char *user = getenv("USER");
	if (user != nullptr)
		m_sUsername = {user};
	else
	{
		struct passwd *pwd = getpwuid(getuid());
		if (pwd != nullptr)
			m_sUsername = {pwd->pw_name};
	}
#endif
	// fallback
	if (m_sUsername.isEmpty())
		m_sUsername = {PACKAGE_NAME "-user"};
	return m_sUsername;
}

// i.e. toplevel appdata path
UString Environment::getUserDataPath()
{
	if (!m_sAppDataPath.isEmpty())
		return m_sAppDataPath;

	char *path = SDL_GetPrefPath("", "");
	if (path != NULL)
	{
		m_sAppDataPath = {path};
		// since this is kind of an abuse of SDL_GetPrefPath, we remove the double slash
		if (m_sAppDataPath.endsWith(Env::cfg(OS::WINDOWS) ? "\\\\" : "//"))
			m_sAppDataPath.erase(m_sAppDataPath.length() - 1, 1);
	}

	SDL_free(path);

	if (m_sAppDataPath.isEmpty())
		m_sAppDataPath = Env::cfg(OS::WINDOWS) ? "C:\\" : "/"; // TODO: fallback?

	return m_sAppDataPath;
}

// i.e. ~/.local/share/PACKAGE_NAME
UString Environment::getLocalDataPath()
{
	if (!m_sProgDataPath.isEmpty())
		return m_sProgDataPath;

	char *path = SDL_GetPrefPath("McEngine", PACKAGE_NAME);
	if (path != NULL)
		m_sProgDataPath = {path};

	SDL_free(path);

	if (m_sProgDataPath.isEmpty()) // fallback to exe dir
		m_sProgDataPath = getExecutablePath();

	return m_sProgDataPath;
}

bool Environment::fileExists(UString filename) const
{
	SDL_PathInfo info;
	if (SDL_GetPathInfo(filename.toUtf8(), &info))
		return info.type == SDL_PATHTYPE_FILE;

	return false;
}

bool Environment::directoryExists(UString directoryName) const
{
	SDL_PathInfo info;
	if (SDL_GetPathInfo(directoryName.toUtf8(), &info))
		return info.type == SDL_PATHTYPE_DIRECTORY;

	return false;
}

bool Environment::createDirectory(UString directoryName) const
{
	return SDL_CreateDirectory(directoryName.toUtf8());
}

bool Environment::renameFile(UString oldFileName, UString newFileName)
{
	return std::rename(oldFileName.toUtf8(), newFileName.toUtf8()); // TODO: use SDL for this?
}

bool Environment::deleteFile(UString filePath)
{
	return std::remove(filePath.toUtf8()) == 0; // TODO: maybe use SDL for this?
}

std::vector<UString> Environment::getFilesInFolder(UString folder) const
{
	return enumerateDirectory(folder.toUtf8(), SDL_PATHTYPE_FILE);
}

std::vector<UString> Environment::getFoldersInFolder(UString folder) const
{
	// TODO: if this turns out to be too slow for folders with a lot of subfolders, split out the sorting
	// currently only the skinlist really uses it, shouldn't have more than 5000 skins in it for normal human beings
	auto folders = enumerateDirectory(folder.toUtf8(), SDL_PATHTYPE_DIRECTORY);
	if (!Env::cfg(OS::WINDOWS))
		winSortInPlace(folders);
	return folders;
}

// sadly, sdl doesn't give a way to do this
std::vector<UString> Environment::getLogicalDrives() const
{
	std::vector<UString> drives{};

	if constexpr (Env::cfg(OS::LINUX))
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
					drives.emplace_back(drivePath);
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

UString Environment::getFolderFromFilePath(UString filepath) const
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

UString Environment::getFileExtensionFromFilePath(UString filepath, bool includeDot) const
{
	const int idx = filepath.findLast(".");
	if (idx != -1)
		return filepath.substr(idx + 1);
	else
		return UString("");
}

UString Environment::getFileNameFromFilePath(UString filepath) const
{
	if (filepath.length() < 1)
		return filepath;

	const size_t lastSlashIndex = filepath.findLast("/");
	if (lastSlashIndex != std::string::npos)
		return filepath.substr(lastSlashIndex + 1);

	return filepath;
}

UString Environment::getClipBoardText()
{
	if (m_sPrevClipboardTextSDL != NULL)
		SDL_free((void *)m_sPrevClipboardTextSDL);

	m_sPrevClipboardTextSDL = SDL_GetClipboardText();

	return (m_sPrevClipboardTextSDL != NULL ? UString(m_sPrevClipboardTextSDL) : UString(""));
}

void Environment::setClipBoardText(UString text)
{
	SDL_SetClipboardText(text.toUtf8());
}

void Environment::showMessageInfo(UString title, UString message) const
{
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, title.toUtf8(), message.toUtf8(), m_window);
}

void Environment::showMessageWarning(UString title, UString message) const
{
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, title.toUtf8(), message.toUtf8(), m_window);
}

void Environment::showMessageError(UString title, UString message) const
{
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title.toUtf8(), message.toUtf8(), m_window);
}

void Environment::showMessageErrorFatal(UString title, UString message) const
{
	showMessageError(title, message);
}

// WIP
void Environment::fileDialogCallback(void *userdata, const char *const *filelist, int filter)
{
	if (filelist && *filelist)
	{
		s_fileDialogState.result = UString(*filelist);
		if (userdata != NULL)
			static_cast<ConVar *>(userdata)->setValue(*filelist); // TODO: make this less fragile (convars are a bad way to handle this, e.g. multiple files???)
	}
	else
	{
		s_fileDialogState.result = "";
	}
	s_fileDialogState.done = true;
}

// TODO: result unused, needs a way to signal the callback like openFolderWindow
void Environment::openFileWindow(const char *filetypefilters, UString title, UString initialpath) const
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

	return;
}

// TODO: what if multiple of these could be opened at once? or openFolderWindow + openFileWindow?
void Environment::openFolderWindow(ConVar &callback, UString initialpath) const
{
	// reset static file dialog state
	s_fileDialogState.done = false;
	s_fileDialogState.result = "";

	// show it
	SDL_ShowOpenFolderDialog(fileDialogCallback, &callback, m_window, initialpath.length() > 0 ? initialpath.toUtf8() : nullptr, false);

	return;
}

void Environment::focus()
{
	SDL_RaiseWindow(m_window);
	m_bHasFocus = true;
}

void Environment::center()
{
	SDL_SyncWindow(m_window);
	const SDL_DisplayID di = SDL_GetDisplayForWindow(m_window);
	SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED_DISPLAY(di), SDL_WINDOWPOS_CENTERED_DISPLAY(di));
}

void Environment::minimize()
{
	SDL_MinimizeWindow(m_window);
	m_bHasFocus = false;
}

void Environment::maximize()
{
	SDL_MaximizeWindow(m_window);
}

void Environment::enableFullscreen()
{
	if (m_bFullscreen)
		return;
	if ((m_bFullscreen = SDL_SetWindowFullscreen(m_window, true))) // NOTE: "fake" fullscreen since we don't want a videomode change
		return;
	// if (m_sdlDebug) debugLog("%s %s\n", __PRETTY_FUNCTION__, SDL_GetError());
}

void Environment::disableFullscreen()
{
	if (!(m_bFullscreen = !SDL_SetWindowFullscreen(m_window, false)))
		return;
	// if (m_sdlDebug) debugLog("%s %s\n", __PRETTY_FUNCTION__, SDL_GetError());
}

void Environment::setFullscreenWindowedBorderless(bool fullscreenWindowedBorderless)
{
	m_bFullscreenWindowedBorderless = fullscreenWindowedBorderless;

	if (isFullscreen())
	{
		disableFullscreen();
		enableFullscreen();
	}
}

void Environment::setWindowTitle(UString title)
{
	SDL_SetWindowTitle(m_window, title.toUtf8());
}

void Environment::setWindowPos(int x, int y)
{
	SDL_SetWindowPosition(m_window, x, y);
}

void Environment::setWindowSize(int width, int height)
{
	SDL_SetWindowSize(m_window, width, height);
}

void Environment::setWindowResizable(bool resizable)
{
	SDL_SetWindowResizable(m_window, resizable);
	m_bResizable = resizable;
}

void Environment::setWindowGhostCorporeal(bool corporeal)
{
	// TODO (wtf is this?)
}

void Environment::setMonitor(int monitor)
{
	// TODO:
	center();
}

HWND Environment::getHwnd() const
{
	HWND hwnd = nullptr;
#if defined(MCENGINE_PLATFORM_WINDOWS)
    hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(m_window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
    if (!hwnd)
        debugLog("(Windows) hwnd is null! SDL: %s\n", SDL_GetError());
#elif defined(__MACOS__)
#error "no macos support currently"
    NSWindow *nswindow = (__bridge NSWindow *)SDL_GetPointerProperty(SDL_GetWindowProperties(m_window), SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL);
    if (nswindow) {
    }
#elif defined(MCENGINE_PLATFORM_LINUX)
    if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "x11") == 0) {
        auto *xdisplay = (Display *)SDL_GetPointerProperty(SDL_GetWindowProperties(m_window), SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL);
        auto xwindow = (Window)SDL_GetNumberProperty(SDL_GetWindowProperties(m_window), SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
        if (xdisplay && xwindow)
            hwnd = (HWND)xwindow;
		else
			debugLog("(X11) no display/no surface! SDL: %s\n", SDL_GetError());
    } else if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "wayland") == 0) {
        struct wl_display *display = (struct wl_display *)SDL_GetPointerProperty(SDL_GetWindowProperties(m_window), SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, NULL);
        struct wl_surface *surface = (struct wl_surface *)SDL_GetPointerProperty(SDL_GetWindowProperties(m_window), SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, NULL);
        if (display && surface)
            hwnd = (HWND)surface;
		else
			debugLog("(Wayland) no display/no surface! SDL: %s\n", SDL_GetError());
    }
#endif

	return hwnd;
}

Vector2 Environment::getWindowPos() const
{
	int x = 0;
	int y = 0;
	SDL_GetWindowPosition(m_window, &x, &y);
	return {static_cast<float>(x), static_cast<float>(y)};
}

Vector2 Environment::getWindowSize() const
{
	int width = 100;
	int height = 100;
	SDL_GetWindowSize(m_window, &width, &height);
	return {static_cast<float>(width), static_cast<float>(height)};
}

int Environment::getMonitor() const
{
	const int display = static_cast<int>(SDL_GetDisplayForWindow(m_window));
	return display - 1 < 0 ? 0 : display - 1; // HACK: 0 means invalid display in SDL, decrement by 1 for engine/app
}

Vector2 Environment::getNativeScreenSize() const
{
	SDL_DisplayID di = SDL_GetDisplayForWindow(m_window);
	return {static_cast<float>(SDL_GetDesktopDisplayMode(di)->w), static_cast<float>(SDL_GetDesktopDisplayMode(di)->h)};
}

McRect Environment::getVirtualScreenRect() const
{
	// TODO:
	return {0, 0, 1, 1};
}

McRect Environment::getDesktopRect() const
{
	return {{0, 0}, getNativeScreenSize()};
}

McRect Environment::getWindowRect() const
{
	return {getWindowPos(), getWindowSize()};
}

int Environment::getDPI() const
{
	float dpi = SDL_GetWindowDisplayScale(m_window) * 96;

	return std::clamp<int>((int)dpi, 96, 96 * 2); // sanity clamp
}

Vector2 Environment::getMousePos() const
{
	return m_vLastAbsMousePos;
}

void Environment::setCursor(CURSORTYPE cur)
{
	if (m_cursorType != cur)
	{
		m_cursorType = cur;
		SDL_SetCursor(m_mCursorIcons.at(m_cursorType)); // does not make visible if the cursor isn't visible
	}
}

void Environment::setCursorVisible(bool visible)
{
	m_bCursorVisible = visible;
	if (visible)
	{
		// disable rawinput (allow regular mouse movement)
		// TODO: consolidate all this BS transition logic into some onPointerEnter/onPointerLeave handler
		if (m_bIsRawInput)
		{
			setRawInput(false);
			setCursorPosition(getMousePos().nudge(getWindowSize()/2, 1.0f)); // nudge it outwards
		}
		else // snap the OS cursor to virtual cursor position
			setCursorPosition(m_engine->getMouse()->getRealPos().nudge(getWindowSize()/2, 1.0f)); // nudge it outwards
		SDL_ShowCursor();
	}
	else
	{
		setCursor(CURSORTYPE::CURSOR_NORMAL);
		SDL_HideCursor();
		if (m_bIsRawInput) // re-enable rawinput
			setRawInput(true);
	}
}

void Environment::setCursorClip(bool clip, McRect rect)
{
	m_cursorClip = rect;
	if (clip)
	{
		if (m_bIsRawInput)
		{
			const SDL_Rect clipRect{static_cast<int>(rect.getX()), static_cast<int>(rect.getY()), static_cast<int>(rect.getWidth()), static_cast<int>(rect.getHeight())};
			SDL_SetWindowMouseRect(m_window, &clipRect);
		}
		SDL_SetWindowMouseGrab(m_window, true);
		m_bCursorClipped = true;
	}
	else
	{
		m_bCursorClipped = false;
		SDL_SetWindowMouseRect(m_window, NULL);
		SDL_SetWindowMouseGrab(m_window, false);
	}
}

UString Environment::keyCodeToString(KEYCODE keyCode)
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

void Environment::listenToTextInput(bool listen)
{
	listen ? SDL_StartTextInput(m_window) : SDL_StopTextInput(m_window);
	SDL_SetWindowKeyboardGrab(m_window, !listen);
}

//******************************//
//	internal helpers/callbacks  //
//******************************//

void Environment::setRawInput(bool on)
{
	if (on)
	{
		setCursorPosition(m_engine->getMouse()->getRealPos()); // when enabling, we need to make sure we start from the virtual cursor position
		if (m_bCursorClipped)
		{
			const SDL_Rect clipRect{.x=static_cast<int>(m_cursorClip.getX()), .y=static_cast<int>(m_cursorClip.getY()), .w=static_cast<int>(m_cursorClip.getWidth()), .h=static_cast<int>(m_cursorClip.getHeight())};
			SDL_SetWindowMouseRect(m_window, &clipRect);
		}
	}
	else
	{
		// let the mouse handler clip the cursor as it sees fit
		// this is because SDL has no equivalent of sensTransformFunc for non-relative mouse mode
		SDL_SetWindowMouseRect(m_window, NULL);
	}

	SDL_SetRelativeMouseTransform(on ? sensTransformFunc : nullptr, nullptr);
	SDL_SetWindowRelativeMouseMode(m_window, on);
}

void Environment::onRawInputChange(float newval)
{
	m_bIsRawInput = !!static_cast<int>(newval);
	setRawInput(m_bIsRawInput);
}

void Environment::onLogLevelChange(float newval)
{
	const bool enable = !!static_cast<int>(newval);
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

static void sensTransformFunc(void *, Uint64, SDL_Window *, SDL_MouseID, float *x, float *y)
{
	float sensitivity = mouse_sensitivity.getFloat();
	*x *= sensitivity;
	*y *= sensitivity;
}

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
