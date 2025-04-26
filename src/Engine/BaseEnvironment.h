//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		Consteval functions for determining compilation configuration without macros
//
// $NoKeywords: $env
//===============================================================================//

#pragma once
#ifndef BASEENVIRONMENT_H
#define BASEENVIRONMENT_H

#include "config.h"
#include "EngineFeatures.h"
#include <cstdint>
#include <type_traits>

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
	enum class AUD : uint32_t
	{
		BASS	= 1 << 0,
		WASAPI	= 1 << 1,
		SDL		= 1 << 2,
		SOLOUD	= 1 << 3,
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
	constexpr AUD operator|(AUD lhs, AUD rhs) {return static_cast<AUD>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));}
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

	consteval AUD getAudioBackend() {
		return
	#ifdef MCENGINE_FEATURE_BASS
		AUD::BASS |
	#endif
	#ifdef MCENGINE_FEATURE_BASS_WASAPI
		AUD::WASAPI |
	#endif
	#ifdef MCENGINE_FEATURE_SDL_MIXER
		AUD::SDL |
	#endif
	#ifdef MCENGINE_FEATURE_SOLOUD
		AUD::SOLOUD |
	#endif
		AUD::NONE;
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
		} else if constexpr (std::is_same_v<T, AUD>) {
            return (static_cast<uint32_t>(mask) & static_cast<uint32_t>(getAudioBackend())) != 0;
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
using Env::AUD;
using Env::FEAT;

#endif
