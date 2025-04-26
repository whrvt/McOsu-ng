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
namespace Env
{
    enum class OS : uint32_t
    {
        WINDOWS	= 1 << 0,
        LINUX	= 1 << 1,
        MACOS	= 1 << 2,
        HORIZON	= 1 << 3,
		WASM	= 1 << 4,
		NONE	= 0,
	};
	enum class BACKEND : uint32_t
	{
		NATIVE	= 1 << 0,
		SDL		= 1 << 1,
		NONE	= 0,
	};
	enum class FEAT : uint32_t
	{
		JOY	    = 1 << 0,
		JOY_MOU	= 1 << 1,
		TOUCH	= 1 << 2,
		NONE	= 0,
	};
	enum class REND : uint32_t
	{
		GL		= 1 << 0,
		GLES2	= 1 << 1,
		DX11	= 1 << 2,
		SW		= 1 << 3,
		NONE	= 0,
	};

    constexpr OS operator|(OS lhs, OS rhs) {return static_cast<OS>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));}
    constexpr BACKEND operator|(BACKEND lhs, BACKEND rhs) {return static_cast<BACKEND>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));}
	constexpr FEAT operator|(FEAT lhs, FEAT rhs) {return static_cast<FEAT>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));}
    constexpr REND operator|(REND lhs, REND rhs) {return static_cast<REND>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));}

	// system McOsu was compiled for
	consteval OS getOS() {
	#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__CYGWIN__) || defined(__CYGWIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__)
		return OS::WINDOWS;
	#elif defined __linux__
		return OS::LINUX;
	#elif defined __APPLE__
		return OS::MACOS;
	#elif defined __SWITCH__
		return OS::HORIZON;
	#elif defined __EMSCRIPTEN__
		return OS::WASM;
	#else
	#error "Compiling for an unknown target!"
		return OS::NONE;
	#endif
	}

	// environment abstraction backend
	consteval BACKEND getBackend() {
	#ifdef MCENGINE_FEATURE_SDL
		return BACKEND::SDL;
	#elif 1
		return BACKEND::NATIVE;
	#else
		return BACKEND::NONE;
	#endif
	}

	// miscellaneous compile-time features
	consteval FEAT getFeatures() {
		return
	#ifdef MCENGINE_SDL_JOYSTICK
		FEAT::JOY |
	#endif
	#ifdef MCENGINE_SDL_JOYSTICK_MOUSE
		FEAT::JOY_MOU |
	#endif
	#ifdef MCENGINE_SDL_TOUCHSUPPORT
		FEAT::TOUCH |
	#endif
		FEAT::NONE;
	}

	// graphics renderer type (multiple can be enabled at the same time, like DX11 + GL for windows)
	consteval REND getRenderers() {
		return
	#ifdef MCENGINE_FEATURE_OPENGL
		REND::GL |
	#endif
	#ifdef MCENGINE_FEATURE_OPENGLES
		REND::GLES2 |
	#endif
	#ifdef MCENGINE_FEATURE_DIRECTX11
		REND::DX11 |
	#endif
	#ifdef MCENGINE_FEATURE_SOFTRENDERER
		REND::SW |
	#endif
	#if !(defined(MCENGINE_FEATURE_OPENGL) || defined(MCENGINE_FEATURE_OPENGLES) || defined(MCENGINE_FEATURE_DIRECTX11) || defined(MCENGINE_FEATURE_SOFTRENDERER))
	#warning "No renderer is defined! Check \"EngineFeatures.h\"."
	#endif
		REND::NONE;
	}

    template<typename T>
    struct Not {
        T value;
        consteval Not(T v) : value(v) {}
    };

    template<typename T>
    consteval Not<T> operator!(T value) {return Not<T>(value);}

    template<typename T>
    constexpr bool always_false_v = false;

    // check if a specific config mask matches current config
    template<typename T>
    consteval bool matchesCurrentConfig(T mask) {
        if constexpr (std::is_same_v<T, OS>) {
            return (static_cast<uint32_t>(mask) & static_cast<uint32_t>(getOS())) != 0;
        } else if constexpr (std::is_same_v<T, BACKEND>) {
            return (static_cast<uint32_t>(mask) & static_cast<uint32_t>(getBackend())) != 0;
		} else if constexpr (std::is_same_v<T, FEAT>) {
            return (static_cast<uint32_t>(mask) & static_cast<uint32_t>(getFeatures())) != 0;
        } else if constexpr (std::is_same_v<T, REND>) {
            return (static_cast<uint32_t>(mask) & static_cast<uint32_t>(getRenderers())) != 0;
        } else {
            static_assert(always_false_v<T>, "Unsupported type for cfg");
            return false;
        }
    }

	// specialization for !<mask>
    template<typename T>
    consteval bool matchesCurrentConfig(Not<T> not_mask) {return !matchesCurrentConfig(not_mask.value);}

    // base case
    consteval bool cfg() {return true;}
    // recursive case for variadic template
    template<typename T, typename... Rest>
    consteval bool cfg(T first, Rest... rest) {return matchesCurrentConfig(first) && cfg(rest...);}
}

using Env::OS;
using Env::BACKEND;
using Env::REND;
using Env::FEAT;

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
	virtual UString getExecutablePath() = 0;
	virtual void openURLInDefaultBrowser(UString url) = 0;

	// user
	virtual UString getUsername() = 0;
	virtual UString getUserDataPath() = 0;

	// file IO
	virtual bool fileExists(UString fileName) = 0;
	virtual bool directoryExists(UString directoryName) = 0;
	virtual bool createDirectory(UString directoryName) = 0;
	virtual bool renameFile(UString oldFileName, UString newFileName) = 0;
	virtual bool deleteFile(UString filePath) = 0;
	virtual std::vector<UString> getFilesInFolder(UString folder) = 0;
	virtual std::vector<UString> getFoldersInFolder(UString folder) = 0;
	virtual std::vector<UString> getLogicalDrives() = 0;
	virtual UString getFolderFromFilePath(UString filepath) = 0;
	virtual UString getFileExtensionFromFilePath(UString filepath, bool includeDot = false) = 0;
	virtual UString getFileNameFromFilePath(UString filePath) = 0;

	// clipboard
	virtual UString getClipBoardText() = 0;
	virtual void setClipBoardText(UString text) = 0;

	// dialogs & message boxes
	virtual void showMessageInfo(UString title, UString message) = 0;
	virtual void showMessageWarning(UString title, UString message) = 0;
	virtual void showMessageError(UString title, UString message) = 0;
	virtual void showMessageErrorFatal(UString title, UString message) = 0;
	virtual UString openFileWindow(const char *filetypefilters, UString title, UString initialpath) = 0;
	virtual UString openFolderWindow(UString title, UString initialpath) = 0;

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
	virtual Vector2 getWindowPos() = 0;
	virtual Vector2 getWindowSize() = 0;
	virtual int getMonitor() = 0;
	virtual std::vector<McRect> getMonitors() = 0;
	virtual Vector2 getNativeScreenSize() = 0;
	virtual McRect getVirtualScreenRect() = 0;
	virtual McRect getDesktopRect() = 0;
	virtual int getDPI() = 0;
	virtual bool isFullscreen() = 0;
	virtual bool isWindowResizable() = 0;

	// mouse
	virtual bool isCursorInWindow() = 0;
	virtual bool isCursorVisible() = 0;
	virtual bool isCursorClipped() = 0;
	virtual Vector2 getMousePos() = 0;
	virtual McRect getCursorClip() = 0;
	virtual CURSORTYPE getCursor() = 0;
	virtual void setCursor(CURSORTYPE cur) = 0;
	virtual void setCursorVisible(bool visible) = 0;
	virtual void setMousePos(int x, int y) = 0;
    virtual void setMousePos(float x, float y) = 0;
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
	static ConVar *debug_env;

	bool m_bFullscreenWindowedBorderless;
};

extern Environment *env;

#endif
