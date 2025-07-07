//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		Build-time configuration detection utilities and definitions
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
		WASM	= 1 << 2,
		NONE	= 0,
	};
	enum class FEAT : uint32_t
	{
		STEAM	= 1 << 0,
		DISCORD = 1 << 1,
		MAINCB	= 1 << 2,
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
		GLES32	= 1 << 1,
		GL3     = 1 << 2,
		DX11	= 1 << 3,
		NONE	= 0,
	};

    constexpr OS operator|(OS lhs, OS rhs) {return static_cast<OS>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));}
	constexpr FEAT operator|(FEAT lhs, FEAT rhs) {return static_cast<FEAT>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));}
	constexpr AUD operator|(AUD lhs, AUD rhs) {return static_cast<AUD>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));}
    constexpr REND operator|(REND lhs, REND rhs) {return static_cast<REND>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));}

	// system McOsu was compiled for
	consteval OS getOS() {
	#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__CYGWIN__) || defined(__CYGWIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__)
		return OS::WINDOWS;
	#elif defined __linux__
		return OS::LINUX;
	#elif defined __EMSCRIPTEN__
		return OS::WASM;
	#else
	#error "Compiling for an unknown target!"
		return OS::NONE;
	#endif
	}

	// miscellaneous compile-time features
	consteval FEAT getFeatures() {
		return
	#ifdef MCENGINE_FEATURE_STEAMWORKS
		FEAT::STEAM |
	#endif
	#if defined(MCENGINE_PLATFORM_WASM) || defined(MCENGINE_FEATURE_MAINCALLBACKS)
		FEAT::MAINCB |
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
	#ifdef MCENGINE_FEATURE_GLES32
		REND::GLES32 |
	#endif
	#ifdef MCENGINE_FEATURE_GL3
		REND::GL3 |
	#endif
	#ifdef MCENGINE_FEATURE_DIRECTX11
		REND::DX11 |
	#endif
	#if !(defined(MCENGINE_FEATURE_OPENGL) || defined(MCENGINE_FEATURE_GLES32) || defined(MCENGINE_FEATURE_GL3) || defined(MCENGINE_FEATURE_DIRECTX11))
	#error "No renderer is defined! Check \"EngineFeatures.h\"."
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
using Env::REND;
using Env::AUD;
using Env::FEAT;

#ifdef __AVX512F__
static constexpr auto OPTIMAL_UNROLL = 10;
#elif defined(__AVX2__)
static constexpr auto OPTIMAL_UNROLL = 8;
#elif defined(__SSE2__)
static constexpr auto OPTIMAL_UNROLL = 6;
#else
static constexpr auto OPTIMAL_UNROLL = 4;
#endif

#if defined(__GNUC__) || defined(__clang__)
#define likely(x) __builtin_expect(bool(x),1)
#define unlikely(x) __builtin_expect(bool(x),0)
#define forceinline __attribute__((always_inline)) inline
#define MC_DO_PRAGMA(x) _Pragma (#x)

#ifdef __clang__
#define MC_VECTORIZE_LOOP _Pragma("clang loop vectorize(enable)")
#define MC_UNR_cnt(num) MC_DO_PRAGMA(clang loop unroll_count(num))
#define NULL_PUSH
#define NULL_POP
#else
#define MC_VECTORIZE_LOOP _Pragma("GCC ivdep")
#define MC_UNR_cnt(num) MC_DO_PRAGMA(GCC unroll num)
#define NULL_PUSH MC_DO_PRAGMA(GCC diagnostic ignored "-Wformat") MC_DO_PRAGMA(GCC diagnostic push)
#define NULL_POP MC_DO_PRAGMA(GCC diagnostic pop)
#endif

#define MC_VEC_UNR_cnt(num) MC_VECTORIZE_LOOP MC_UNR_cnt(num)
#define MC_UNROLL_VECTOR MC_VEC_UNR_cnt(OPTIMAL_UNROLL)
#define MC_UNROLL MC_UNR_cnt(OPTIMAL_UNROLL)

#ifdef _OPENMP
#define ACCUMULATE(op, var) MC_DO_PRAGMA(omp simd reduction(op:var)) // use openmp if available, otherwise unroll
#else
#define ACCUMULATE(op, var) MC_UNR_cnt(OPTIMAL_UNROLL)
#endif

#else

#define likely(x) (x)
#define unlikely(x) (x)
#define forceinline __forceinline
#define MC_DO_PRAGMA(x)
#define MC_VECTORIZE_LOOP
#define MC_UNR_cnt(num)
#define MC_VEC_UNR_cnt(num)
#define MC_UNROLL_VECTOR
#define MC_UNROLL
#define NULL_PUSH
#define NULL_POP
#define ACCUMULATE(op, var)
#endif // defined(__GNUC__) || defined(__clang__)

#if !(defined(MCENGINE_PLATFORM_WINDOWS) || defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__CYGWIN__) || defined(__CYGWIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__))
typedef void* HWND;
#else

// umm what the fuck
#if defined(_MSC_VER)

#ifdef _WIN64
#define _AMD64_
#elif defined(_WIN32)
#define _X86_
#endif

#endif

#include "WinDebloatDefs.h"

#include <basetsd.h>
#include <windef.h>
#if defined(_MSC_VER)
typedef SSIZE_T ssize_t;
#endif
#ifndef fileno
#define fileno _fileno
#endif
#ifndef isatty
#define isatty _isatty
#endif
#endif

#endif
