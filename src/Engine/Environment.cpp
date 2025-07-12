//========== Copyright (c) 2018, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		top level interface for native OS calls
//
// $NoKeywords: $env
//===============================================================================//

#include "Environment.h"

#include "Engine.h"
#include "Mouse.h"

#include "DirectX11Interface.h"
#include "SDLGLInterface.h"

#include "File.h"

#if defined(MCENGINE_PLATFORM_WINDOWS)
#include <io.h>
#include <lmcons.h>
#include <windows.h>
#elif defined(__APPLE__) || defined(MCENGINE_PLATFORM_LINUX)
#include <pwd.h>
#include <unistd.h>
#ifdef MCENGINE_PLATFORM_LINUX
#include <X11/Xlib.h>
#endif
#elif defined(__EMSCRIPTEN__)
// TODO
#endif

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <utility>

namespace cv
{
// definitions
ConVar debug_env("debug_env", false, FCVAR_NONE);
ConVar fullscreen_windowed_borderless("fullscreen_windowed_borderless", false, FCVAR_NONE);
ConVar monitor("monitor", 0, FCVAR_NONE, "monitor/display device to switch to, 0 = primary monitor");

ConVar processpriority("processpriority", 0, FCVAR_NONE, "sets the main process priority (0 = normal, 1 = high)", [](float newValue) -> void {
	SDL_SetCurrentThreadPriority(!!static_cast<int>(newValue) ? SDL_THREAD_PRIORITY_HIGH : SDL_THREAD_PRIORITY_NORMAL);
});
} // namespace cv

Environment *env = nullptr;

bool Environment::s_bIsATTY = false;
bool Environment::s_bIsWine = false;

Environment::Environment(int argc, char *argv[])
{
	env = this;

	// parse args
	m_mArgMap = [&]() {
		// example usages:
		// args.contains("-file")
		// auto filename = args["-file"].value_or("default.txt");
		// if (args["-output"].has_value())
		// 	auto outfile = args["-output"].value();
		std::unordered_map<UString, std::optional<UString>> args;
		for (int i = 1; i < argc; ++i)
		{
			std::string_view arg = argv[i];
			if (arg.starts_with('-'))
				if (i + 1 < argc && !(argv[i + 1][0] == '-'))
				{
					args[UString(arg)] = argv[i + 1];
					++i;
				}
				else
					args[UString(arg)] = std::nullopt;
			else
				args[UString(arg)] = std::nullopt;
		}
		return args;
	}();

	// simple vector representation of the whole cmdline including the program name (as the first element)
	m_vCmdLine = std::vector<UString>(argv, argv + argc);

	s_bIsATTY = ::isatty(fileno(stdout)) != 0;
#ifdef MCENGINE_PLATFORM_WINDOWS
	s_bIsWine = !!GetProcAddress(GetModuleHandle(TEXT("ntdll.dll")), "wine_get_version");
#endif

	m_engine = nullptr; // will be initialized by the mainloop once setup is complete
	m_window = nullptr;

	m_bRunning = true;
	m_bDrawing = false;

	m_bMinimized = false; // for fps_max_background
	m_bHasFocus = true;   // for fps_max_background
	m_bFullscreenWindowedBorderless = false;

	m_fDisplayHz = 360.0f;
	m_fDisplayHzSecs = 1.0f / m_fDisplayHz;

	m_bEnvDebug = false;

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

	// lazy init
	m_mCursorIcons = {};

	m_vLastAbsMousePos = Vector2{};
	m_vLastRelMousePos = Vector2{};

	m_sCurrClipboardText = {};
	// lazy init
	m_mMonitors = {};

	// setup callbacks
	cv::debug_env.setCallback(fastdelegate::MakeDelegate(this, &Environment::onLogLevelChange));
	cv::fullscreen_windowed_borderless.setCallback(fastdelegate::MakeDelegate(this, &Environment::onFullscreenWindowBorderlessChange));
	cv::monitor.setCallback(fastdelegate::MakeDelegate(this, &Environment::onMonitorChange));
}

Environment::~Environment()
{
	for (auto &cur : m_mCursorIcons)
	{
		SDL_DestroyCursor(cur.second);
	}
	env = nullptr;
}

// called by mainloop when initialization is ready
Engine *Environment::initEngine()
{
	return new Engine();
}

// well this doesn't do much atm... called at the end of engine->onUpdate
void Environment::update()
{
	// Environment::update();

	m_bIsCursorInsideWindow = m_bHasFocus && McRect(0, 0, m_engine->getScreenWidth(), m_engine->getScreenHeight()).contains(getMousePos());
}

Graphics *Environment::createRenderer()
{
#ifndef MCENGINE_FEATURE_DIRECTX11
	// need to load stuff dynamically before the base class constructors
	SDLGLInterface::load();
	return new SDLGLInterface(m_window);
#else
	return new DirectX11Interface(Env::cfg(OS::WINDOWS) ? getHwnd() : reinterpret_cast<HWND>(m_window));
#endif
}

void Environment::shutdown()
{
	SDL_Event event;
	event.type = SDL_EVENT_QUIT;
	SDL_PushEvent(&event);
}

// TODO
void Environment::restart()
{
	// SAFE_DELETE(m_engine);
	// m_engine = new Engine();
	shutdown();
}

UString Environment::getExecutablePath()
{
	const char *path = SDL_GetBasePath();
	if (!path)
		return {Env::cfg(OS::WINDOWS) ? ".\\" : "./"};

	return {path};
}

void Environment::openURLInDefaultBrowser(const UString &url)
{
	if (!SDL_OpenURL(url.toUtf8()))
		debugLog("Failed to open URL: {:s}\n", SDL_GetError());
}

const UString &Environment::getUsername()
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
#ifndef MCENGINE_PLATFORM_WASM
	else
	{
		struct passwd *pwd = getpwuid(getuid());
		if (pwd != nullptr)
			m_sUsername = {pwd->pw_name};
	}
#endif
#endif
	// fallback
	if (m_sUsername.isEmpty())
		m_sUsername = {PACKAGE_NAME "-user"};
	return m_sUsername;
}

// i.e. toplevel appdata path
const UString &Environment::getUserDataPath()
{
	if (!m_sAppDataPath.isEmpty())
		return m_sAppDataPath;

	char *path = SDL_GetPrefPath("", "");
	if (path != nullptr)
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
const UString &Environment::getLocalDataPath()
{
	if (!m_sProgDataPath.isEmpty())
		return m_sProgDataPath;

	char *path = SDL_GetPrefPath("McEngine", PACKAGE_NAME);
	if (path != nullptr)
		m_sProgDataPath = {path};

	SDL_free(path);

	if (m_sProgDataPath.isEmpty()) // fallback to exe dir
		m_sProgDataPath = getExecutablePath();

	return m_sProgDataPath;
}

// modifies the input filename! (checks case insensitively past the last slash)
bool Environment::fileExists(UString &filename)
{
	return McFile::existsCaseInsensitive(filename) == McFile::FILETYPE::FILE;
}

// modifies the input directoryName! (checks case insensitively past the last slash)
bool Environment::directoryExists(UString &directoryName)
{
	return McFile::existsCaseInsensitive(directoryName) == McFile::FILETYPE::FOLDER;
}

// same as the above, but for string literals (so we can't check insensitively and modify the input)
bool Environment::fileExists(const UString &filename)
{
	return McFile::exists(filename) == McFile::FILETYPE::FILE;
}

bool Environment::directoryExists(const UString &directoryName)
{
	return McFile::exists(directoryName) == McFile::FILETYPE::FOLDER;
}

bool Environment::createDirectory(const UString &directoryName)
{
	return SDL_CreateDirectory(directoryName.toUtf8());
}

bool Environment::renameFile(const UString &oldFileName, const UString &newFileName)
{
	return std::rename(oldFileName.toUtf8(), newFileName.toUtf8()); // TODO: use SDL for this?
}

bool Environment::deleteFile(const UString &filePath)
{
	return std::remove(filePath.toUtf8()) == 0; // TODO: maybe use SDL for this?
}

std::vector<UString> Environment::getFilesInFolder(const UString &folder)
{
	return enumerateDirectory(folder.toUtf8(), SDL_PATHTYPE_FILE);
}

std::vector<UString> Environment::getFoldersInFolder(const UString &folder)
{
	// TODO: if this turns out to be too slow for folders with a lot of subfolders, split out the sorting
	// currently only the skinlist really uses it, shouldn't have more than 5000 skins in it for normal human beings
	auto folders = enumerateDirectory(folder.toUtf8(), SDL_PATHTYPE_DIRECTORY);
	if (!Env::cfg(OS::WINDOWS))
		winSortInPlace(folders);
	return folders;
}

// sadly, sdl doesn't give a way to do this
std::vector<UString> Environment::getLogicalDrives()
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

UString Environment::getFileNameFromFilePath(const UString &filepath) noexcept
{
	return getThingFromPathHelper(filepath, false);
}

UString Environment::getFolderFromFilePath(const UString &filepath) noexcept
{
	return getThingFromPathHelper(filepath, true);
}

UString Environment::getFileExtensionFromFilePath(const UString &filepath, bool /*includeDot*/)
{
	const int idx = filepath.findLast(".");
	if (idx != -1)
		return filepath.substr(idx + 1);
	else
		return UString("");
}

const UString &Environment::getClipBoardText()
{
	char *newClip = SDL_GetClipboardText();
	if (newClip)
		m_sCurrClipboardText = newClip;

	SDL_free(newClip);

	return m_sCurrClipboardText;
}

void Environment::setClipBoardText(const UString &text)
{
	m_sCurrClipboardText = text;
	SDL_SetClipboardText(text.toUtf8());
}

void Environment::showMessageInfo(const UString &title, const UString &message) const
{
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, title.toUtf8(), message.toUtf8(), m_window);
}

void Environment::showMessageWarning(const UString &title, const UString &message) const
{
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, title.toUtf8(), message.toUtf8(), m_window);
}

void Environment::showMessageError(const UString &title, const UString &message) const
{
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title.toUtf8(), message.toUtf8(), m_window);
}

void Environment::showMessageErrorFatal(const UString &title, const UString &message) const
{
	showMessageError(title, message);
}

// TODO: filter?
void Environment::sdlFileDialogCallback(void *userdata, const char *const *filelist, int filter)
{
	auto *callbackData = static_cast<FileDialogCallbackData *>(userdata);
	if (!callbackData)
		return;

	std::vector<UString> results;

	if (filelist)
	{
		for (const char *const *curr = filelist; *curr; curr++)
		{
			results.emplace_back(*curr);
		}
	}

	// call the callback
	callbackData->callback(results);

	// data is no longer needed
	delete callbackData;
}

void Environment::openFileWindow(FileDialogCallback callback, const char *filetypefilters, const UString & /*title*/, const UString &initialpath) const
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

	// callback data to be passed to SDL
	auto *callbackData = new FileDialogCallbackData{std::move(callback)};

	// show it
	SDL_ShowOpenFileDialog(sdlFileDialogCallback, callbackData, m_window, sdlFilters.empty() ? nullptr : sdlFilters.data(), static_cast<int>(sdlFilters.size()),
	                       initialpath.length() > 0 ? initialpath.toUtf8() : nullptr, false);
}

void Environment::openFolderWindow(FileDialogCallback callback, const UString &initialpath) const
{
	// callback data to be passed to SDL
	auto *callbackData = new FileDialogCallbackData{std::move(callback)};

	// show it
	SDL_ShowOpenFolderDialog(sdlFileDialogCallback, callbackData, m_window, initialpath.length() > 0 ? initialpath.toUtf8() : nullptr, false);
}

// just open the file manager in a certain folder, but not do anything with it
void Environment::openFileBrowser(const UString & /*title*/, UString initialpath) const noexcept
{
	UString pathToOpen = std::move(initialpath);
	if (pathToOpen.isEmpty())
		pathToOpen = getExecutablePath();
	else
		pathToOpen = getFolderFromFilePath(pathToOpen);

	// prepend with file:/// to open it as a URI
	if constexpr (Env::cfg(OS::WINDOWS))
		pathToOpen = UString::fmt("file:///{}", pathToOpen);
	else
	{
		if (pathToOpen[0] != '/')
			pathToOpen = UString::fmt("file:///{}", pathToOpen);
		else
			pathToOpen = UString::fmt("file://{}", pathToOpen);
	}

	if (!SDL_OpenURL(pathToOpen.toUtf8()))
		debugLog("Failed to open file URI {:s}: {:s}\n", pathToOpen, SDL_GetError());
}

void Environment::focus()
{
	SDL_RaiseWindow(m_window);
	m_bHasFocus = true;
}

void Environment::center()
{
	syncWindow();
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

// TODO: implement exclusive fullscreen for dx11 backend
void Environment::enableFullscreen()
{
	if (m_bFullscreen)
		return;
	if ((m_bFullscreen = SDL_SetWindowFullscreen(m_window, true))) // NOTE: "fake" fullscreen since we don't want a videomode change
		return;
	// if (m_envDebug) debugLog("{:s} {:s}\n", __PRETTY_FUNCTION__, SDL_GetError());
}

void Environment::disableFullscreen()
{
	if (!(m_bFullscreen = !SDL_SetWindowFullscreen(m_window, false)))
		return;
	// if (m_envDebug) debugLog("{:s} {:s}\n", __PRETTY_FUNCTION__, SDL_GetError());
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

void Environment::setWindowTitle(const UString &title)
{
	SDL_SetWindowTitle(m_window, title.toUtf8());
}

void Environment::syncWindow()
{
	if (m_window)
		SDL_SyncWindow(m_window);
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

void Environment::setMonitor(int monitor)
{
	if (monitor == 0 || monitor == getMonitor())
		return center();

	bool success = false;

	if (!m_mMonitors.contains(monitor)) // try force reinit to check for new monitors
		initMonitors(true);
	if (m_mMonitors.contains(monitor))
	{
		// SDL: "If the window is in an exclusive fullscreen or maximized state, this request has no effect."
		if (m_bFullscreen || m_bFullscreenWindowedBorderless)
		{
			disableFullscreen();
			syncWindow();
			success = SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED_DISPLAY(monitor), SDL_WINDOWPOS_CENTERED_DISPLAY(monitor));
			syncWindow();
			enableFullscreen();
		}
		else
			success = SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED_DISPLAY(monitor), SDL_WINDOWPOS_CENTERED_DISPLAY(monitor));

		if (!success)
			debugLog("WARNING: failed to setMonitor({:d}), centering instead. SDL error: {:s}\n", monitor, SDL_GetError());
		else if (!(success = (monitor == getMonitor())))
			debugLog("WARNING: setMonitor({:d}) didn't actually change the monitor, centering instead.\n", monitor);
	}
	else
		debugLog("WARNING: tried to setMonitor({:d}) to invalid monitor, centering instead\n", monitor);

	if (!success)
		center();
	else
		cv::monitor.setValue(monitor, false);
}

HWND Environment::getHwnd() const
{
	HWND hwnd = nullptr;
#if defined(MCENGINE_PLATFORM_WINDOWS)
	hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(m_window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
	if (!hwnd)
		debugLog("(Windows) hwnd is null! SDL: {:s}\n", SDL_GetError());
#elif defined(__APPLE__)
	NSWindow *nswindow = (__bridge NSWindow *)SDL_GetPointerProperty(SDL_GetWindowProperties(m_window), SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
	if (nswindow)
	{
#warning "getHwnd() TODO"
	}
#elif defined(MCENGINE_PLATFORM_LINUX)
	if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "x11") == 0)
	{
		auto *xdisplay = (Display *)SDL_GetPointerProperty(SDL_GetWindowProperties(m_window), SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
		auto xwindow = (Window)SDL_GetNumberProperty(SDL_GetWindowProperties(m_window), SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
		if (xdisplay && xwindow)
			hwnd = (HWND)xwindow;
		else
			debugLog("(X11) no display/no surface! SDL: {:s}\n", SDL_GetError());
	}
	else if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "wayland") == 0)
	{
		struct wl_display *display =
		    (struct wl_display *)SDL_GetPointerProperty(SDL_GetWindowProperties(m_window), SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
		struct wl_surface *surface =
		    (struct wl_surface *)SDL_GetPointerProperty(SDL_GetWindowProperties(m_window), SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
		if (display && surface)
			hwnd = (HWND)surface;
		else
			debugLog("(Wayland) no display/no surface! SDL: {:s}\n", SDL_GetError());
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

const std::map<unsigned int, McRect> &Environment::getMonitors()
{
	if (m_mMonitors.size() < 1) // lazy init
		initMonitors();
	return m_mMonitors;
}

int Environment::getMonitor() const
{
	const int display = static_cast<int>(SDL_GetDisplayForWindow(m_window));
	return display == 0 ? -1 : display; // 0 == invalid, according to SDL
}

Vector2 Environment::getNativeScreenSize() const
{
	SDL_DisplayID di = SDL_GetDisplayForWindow(m_window);
	return {static_cast<float>(SDL_GetDesktopDisplayMode(di)->w), static_cast<float>(SDL_GetDesktopDisplayMode(di)->h)};
}

McRect Environment::getDesktopRect() const
{
	return {
	    {0, 0},
        getNativeScreenSize()
    };
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

void Environment::setCursor(CURSORTYPE cur)
{
	if (m_mCursorIcons.empty())
		initCursors();
	if (m_cursorType != cur)
	{
		m_cursorType = cur;
		SDL_SetCursor(m_mCursorIcons.at(m_cursorType)); // does not make visible if the cursor isn't visible
	}
}

namespace
{
void sensTransformFunc(void *userdata, Uint64 /*timestamp*/, SDL_Window * /*window*/, SDL_MouseID /*mouseid*/, float *x, float *y)
{
	const float sensitivity = *static_cast<float*>(userdata);
	*x *= sensitivity;
	*y *= sensitivity;
}
} // namespace

void Environment::notifyWantRawInput(bool raw)
{
	if (raw)
	{
		setOSMousePos(mouse->getRealPos()); // when enabling, we need to make sure we start from the virtual cursor position
		if (m_bCursorClipped)
		{
			const SDL_Rect clipRect{.x = static_cast<int>(m_cursorClip.getX()),
			                        .y = static_cast<int>(m_cursorClip.getY()),
			                        .w = static_cast<int>(m_cursorClip.getWidth()),
			                        .h = static_cast<int>(m_cursorClip.getHeight())};
			SDL_SetWindowMouseRect(m_window, &clipRect);
		}
	}
	else
	{
		// let the mouse handler clip the cursor as it sees fit
		// this is because SDL has no equivalent of sensTransformFunc for non-relative mouse mode
		SDL_SetWindowMouseRect(m_window, nullptr);
	}
	static constexpr const float default_sens{1.0f};
	SDL_SetRelativeMouseTransform(raw ? sensTransformFunc : nullptr, mouse ? (void*)&mouse->getSensitivity() : (void*)(&default_sens));
	SDL_SetWindowRelativeMouseMode(m_window, raw);
}

void Environment::setCursorVisible(bool visible)
{
	m_bCursorVisible = visible;
	if (visible)
	{
		// disable rawinput (allow regular mouse movement)
		// TODO: consolidate all this BS transition logic into some onPointerEnter/onPointerLeave handler
		if (mouse->isRawInput())
		{
			notifyWantRawInput(false);
			setOSMousePos(Vector2{getMousePos()}.nudge(getWindowSize() / 2.0f, 1.0f)); // nudge it outwards
		}
		else                                                                        // snap the OS cursor to virtual cursor position
			setOSMousePos(Vector2{mouse->getRealPos()}.nudge(getWindowSize() / 2.0f, 1.0f)); // nudge it outwards
		SDL_ShowCursor();
	}
	else
	{
		setCursor(CURSORTYPE::CURSOR_NORMAL);
		SDL_HideCursor();
		if (mouse->isRawInput()) // re-enable rawinput
			notifyWantRawInput(true);
	}
}

void Environment::setCursorClip(bool clip, McRect rect)
{
	m_cursorClip = rect;
	if (clip)
	{
		if (mouse->isRawInput())
		{
			const SDL_Rect clipRect{.x = static_cast<int>(rect.getX()),
			                        .y = static_cast<int>(rect.getY()),
			                        .w = static_cast<int>(rect.getWidth()),
			                        .h = static_cast<int>(rect.getHeight())};
			SDL_SetWindowMouseRect(m_window, &clipRect);
		}
		SDL_SetWindowMouseGrab(m_window, true);
		m_bCursorClipped = true;
	}
	else
	{
		m_bCursorClipped = false;
		SDL_SetWindowMouseRect(m_window, nullptr);
		SDL_SetWindowMouseGrab(m_window, false);
	}
}

UString Environment::keyCodeToString(KEYCODE keyCode)
{
	const char *name = SDL_GetScancodeName((SDL_Scancode)keyCode);
	if (name == nullptr)
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

void Environment::initCursors()
{
	m_mCursorIcons = {
	    {CURSORTYPE::CURSOR_NORMAL,  SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT)    },
	    {CURSORTYPE::CURSOR_WAIT,    SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT)       },
	    {CURSORTYPE::CURSOR_SIZE_H,  SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_EW_RESIZE)  },
	    {CURSORTYPE::CURSOR_SIZE_V,  SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NS_RESIZE)  },
	    {CURSORTYPE::CURSOR_SIZE_HV, SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NESW_RESIZE)},
	    {CURSORTYPE::CURSOR_SIZE_VH, SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NWSE_RESIZE)},
	    {CURSORTYPE::CURSOR_TEXT,    SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_TEXT)       },
	};
}

void Environment::initMonitors(bool force)
{
	if (!force && !m_mMonitors.empty())
		return;
	else if (force) // refresh
		m_mMonitors.clear();

	int count = -1;
	const SDL_DisplayID *displays = SDL_GetDisplays(&count);

	for (int i = 0; i < count; i++)
	{
		const SDL_DisplayID di = displays[i];
		m_mMonitors.try_emplace(di, McRect{0, 0, static_cast<float>(SDL_GetDesktopDisplayMode(di)->w), static_cast<float>(SDL_GetDesktopDisplayMode(di)->h)});
	}
	if (count < 1)
	{
		debugLog("WARNING: No monitors found! Adding default monitor ...\n");
		const Vector2 windowSize = getWindowSize();
		m_mMonitors.try_emplace(1, McRect{0, 0, windowSize.x, windowSize.y});
	}
}

void Environment::onLogLevelChange(float newval)
{
	const bool enable = !!static_cast<int>(newval);
	if (enable && !m_bEnvDebug)
	{
		envDebug(true);
		SDL_SetLogPriorities(SDL_LOG_PRIORITY_TRACE);
	}
	else if (!enable && m_bEnvDebug)
	{
		envDebug(false);
		SDL_ResetLogPriorities();
	}
}

UString Environment::convertUnixToWindowsPath(const UString &unixPath)
{
	if (!s_bIsWine || unixPath.isEmpty() || unixPath[0] != '/')
		return unixPath;

#if defined(MCENGINE_PLATFORM_WINDOWS)
	// get wine's path conversion function
	typedef LPWSTR (*wine_get_dos_file_name_t)(LPCSTR);
	static wine_get_dos_file_name_t wine_get_dos_file_name_ptr = nullptr;

	if (!wine_get_dos_file_name_ptr)
	{
		wine_get_dos_file_name_ptr = (wine_get_dos_file_name_t)GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "wine_get_dos_file_name");
		if (!wine_get_dos_file_name_ptr)
			return unixPath; // fallback
	}

	// convert to windows path using wine's function
	LPWSTR windows_name = wine_get_dos_file_name_ptr(unixPath.toUtf8());
	if (windows_name)
	{
		UString result(windows_name);
		HeapFree(GetProcessHeap(), 0, windows_name);
		return result;
	}
#endif

	return unixPath; // fallback
}

// folder = true means return the canonical filesystem path to the folder containing the given path
//			if the path is already a folder, just return it directly
// folder = false means to strip away the file path separators from the given path and return just the filename itself
UString Environment::getThingFromPathHelper(UString path, bool folder) noexcept
{
	if (path.isEmpty())
		return path;

	// find the last path separator (either / or \)
	int lastSlash = path.findLast("/");
	if (lastSlash == -1)
		lastSlash = path.findLast("\\");

	if (folder)
	{
		// if path ends with separator, it's already a directory
		bool endsWithSeparator = path.endsWith("/") || path.endsWith("\\");

		std::error_code ec;
		auto abs_path = std::filesystem::canonical(path.plat_str(), ec);

		if (!ec) // canonical path found
		{
			auto status = std::filesystem::status(abs_path, ec);
			// if it's already a directory or it doesn't have a parent path then just return it directly
			if (ec || status.type() == std::filesystem::file_type::directory || !abs_path.has_parent_path())
				path = abs_path.c_str();
			// else return the parent directory for the file
			else if (abs_path.has_parent_path() && !abs_path.parent_path().empty())
				path = abs_path.parent_path().c_str();
		}
		else if (!endsWithSeparator) // canonical failed, handle manually (if it's not already a directory)
		{
			if (lastSlash != -1) // return parent
				path = path.substr(0, lastSlash);
			else // no separators found, just use ./
				path = UString::fmt(".{}{}", Env::cfg(OS::WINDOWS) ? "\\" : "/", path);
		}
		// make sure whatever we got now ends with a slash
		if (!path.endsWith("/") && !path.endsWith("\\"))
			path = path + (Env::cfg(OS::WINDOWS) ? "\\" : "/");

		// convert possible unix paths to windows paths (wine compat)
		if constexpr (Env::cfg(OS::WINDOWS))
			path = convertUnixToWindowsPath(path);
	}
	else if (lastSlash != -1) // just return the file
	{
		path = path.substr(lastSlash + 1);
	}
	// else: no separators found, entire path is the filename

	return path;
}

// for getting files in folder/ folders in folder
std::vector<UString> Environment::enumerateDirectory(const char *pathToEnum, SDL_PathType type)
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
		debugLog("Failed to enumerate directory: {:s}\n", SDL_GetError());

	return contents;
}

#if defined(MCENGINE_PLATFORM_WINDOWS) && !defined(strcasecmp)
#define strcasecmp _stricmp
#endif

// return a more naturally windows-like sorted order for folders, useful for e.g. osu! skin list dropdown order
void Environment::winSortInPlace(std::vector<UString> &toSort)
{
	std::ranges::stable_sort(toSort, [](const UString &a, const UString &b) { return strcasecmp(a.toUtf8(), b.toUtf8()) < 0; });
}
