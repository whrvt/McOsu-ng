//========== Copyright (c) 2015, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		stopwatch/timer
//
// $NoKeywords: $time $chrono
//===============================================================================//

#pragma once
#ifndef TIMER_H
#define TIMER_H

#include "BaseEnvironment.h"

// sdl has better/special handling for emscripten, no point in wasting time on reimplenting that
#ifdef MCENGINE_PLATFORM_WASM
#include <SDL3/SDL_timer.h>
#else
#include <chrono>
#endif

#include <concepts>
#include <cstdint>
#include <thread>

namespace Timing
{
// conversion constants
constexpr uint64_t NS_PER_SECOND = 1'000'000'000;
constexpr uint64_t NS_PER_MS = 1'000'000;
constexpr uint64_t NS_PER_US = 1'000;
constexpr uint64_t US_PER_MS = 1'000;
constexpr uint64_t MS_PER_SECOND = 1'000;

namespace detail
{
#ifndef MCENGINE_PLATFORM_WASM
inline const auto g_initTime = std::chrono::steady_clock::now();
#endif
forceinline void schedyield() noexcept
{
#ifdef MCENGINE_PLATFORM_WASM
	SDL_Delay(0);
#else
	std::this_thread::yield();
#endif
}

forceinline void spinyield() noexcept
{
//clang-format off
#if defined(__GNUC__) || defined(__clang__)
#if defined(__arm__) || defined(__aarch64__) || defined(__arm64ec__)
	__asm__ __volatile__("dmb ishst\n\tyield" : : : "memory");
#elif defined(__i386__) || defined(__x86_64__)
	__asm__ __volatile__("rep; nop" : : : "memory");
#else
	__asm__ __volatile__("" : : : "memory");
#endif
#elif defined(_MSC_VER)
	__asm { rep nop }
	;
#endif
	//clang-format on
}

template <uint64_t Ratio>
constexpr uint64_t convertTime(uint64_t ns) noexcept
{
	return ns / Ratio;
}

} // namespace detail

// get nanoseconds since first timing call (globally) (equivalent to SDL_GetTicksNS)
inline uint64_t getTicksNS() noexcept
{
#ifdef MCENGINE_PLATFORM_WASM
	return SDL_GetTicksNS();
#else
	const auto now = std::chrono::steady_clock::now();
	const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(now - detail::g_initTime);
	return static_cast<uint64_t>(elapsed.count());
#endif
}

// get nanoseconds since first timing call (globally)
constexpr uint64_t ticksNSToMS(uint64_t ns) noexcept
{
	return detail::convertTime<NS_PER_MS>(ns);
}

inline uint64_t getTicksMS() noexcept
{
	return ticksNSToMS(getTicksNS());
}

// inspired by SDL_DelayPrecise
// basically sleep in small increments until we get close to the deadline, then spin
inline void sleepPrecise(uint64_t ns) noexcept
{
#ifdef MCENGINE_PLATFORM_WASM
	SDL_DelayPrecise(ns);
#else
	if (ns == 0) [[unlikely]]
	{
		detail::schedyield();
		return;
	}

	const uint64_t target_time = getTicksNS() + ns;
	constexpr uint64_t COARSE_SLEEP_THRESHOLD = 2 * NS_PER_MS; // 2ms threshold
	constexpr uint64_t COARSE_SLEEP_NS = 1 * NS_PER_MS;        // 1ms coarse sleep

	if (ns > COARSE_SLEEP_THRESHOLD) [[likely]]
	{
		const uint64_t coarse_sleep_duration = ns - COARSE_SLEEP_THRESHOLD;
		std::this_thread::sleep_for(std::chrono::nanoseconds(coarse_sleep_duration));
	}

	uint64_t current_time = getTicksNS();
	while (current_time + COARSE_SLEEP_NS < target_time)
	{
		std::this_thread::sleep_for(std::chrono::nanoseconds(COARSE_SLEEP_NS));
		current_time = getTicksNS();
	}

	while (current_time < target_time)
	{
		detail::spinyield();
		current_time = getTicksNS();
	}
#endif
}

inline void sleep(uint64_t us) noexcept
{
	!!us ? sleepPrecise(us * NS_PER_US) : detail::schedyield();
}

inline void sleepNS(uint64_t ns) noexcept
{
	!!ns ? sleepPrecise(ns) : detail::schedyield();
}

inline void sleepMS(uint64_t ms) noexcept
{
	!!ms ? sleepPrecise(ms * NS_PER_MS) : detail::schedyield();
}

template <typename T = double>
    requires(std::floating_point<T>)
constexpr T timeNSToSeconds(uint64_t ns) noexcept
{
	return static_cast<T>(ns) / static_cast<T>(NS_PER_SECOND);
}

// current time (since init.) in seconds as float
template <typename T = double>
    requires(std::floating_point<T>)
inline T getTimeReal() noexcept
{
	return timeNSToSeconds<T>(getTicksNS());
}

class Timer
{
public:
	explicit Timer(bool startOnCtor = true) noexcept
	{
		if (startOnCtor)
			start();
	}

	~Timer() = default;
	Timer(const Timer &) = default;
	Timer &operator=(const Timer &) = default;
	Timer(Timer &&) = default;
	Timer &operator=(Timer &&) = default;

	inline void start() noexcept
	{
		m_startTimeNS = getTicksNS();
		m_lastUpdateNS = m_startTimeNS;
		m_deltaSeconds = 0.0;
	}

	inline void update() noexcept
	{
		const uint64_t now = getTicksNS();
		m_deltaSeconds = timeNSToSeconds<double>(now - m_lastUpdateNS);
		m_lastUpdateNS = now;
	}

	inline void reset() noexcept
	{
		m_startTimeNS = getTicksNS();
		m_lastUpdateNS = m_startTimeNS;
		m_deltaSeconds = 0.0;
	}

	[[nodiscard]] constexpr double getDelta() const noexcept { return m_deltaSeconds; }

	[[nodiscard]] inline double getElapsedTime() const noexcept { return timeNSToSeconds<double>(m_lastUpdateNS - m_startTimeNS); }

	[[nodiscard]] inline uint64_t getElapsedTimeMS() const noexcept { return ticksNSToMS(m_lastUpdateNS - m_startTimeNS); }

	[[nodiscard]] inline uint64_t getElapsedTimeNS() const noexcept { return m_lastUpdateNS - m_startTimeNS; }

	// get elapsed time without needing update()
	[[nodiscard]] inline double getLiveElapsedTime() const noexcept { return timeNSToSeconds<double>(getTicksNS() - m_startTimeNS); }

	[[nodiscard]] inline uint64_t getLiveElapsedTimeNS() const noexcept { return getTicksNS() - m_startTimeNS; }

private:
	uint64_t m_startTimeNS{};
	uint64_t m_lastUpdateNS{};
	double m_deltaSeconds{};
};

}; // namespace Timing

using Timer = Timing::Timer;

#endif
