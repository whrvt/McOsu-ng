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
#include <thread>
#endif

#include <atomic>
#include <concepts>
#include <cstdint>

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

// get real time from system clock
inline uint64_t getRealTicksNS() noexcept
{
#ifdef MCENGINE_PLATFORM_WASM
	return SDL_GetTicksNS();
#else
	const auto now = std::chrono::steady_clock::now();
	const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(now - g_initTime);
	return static_cast<uint64_t>(elapsed.count());
#endif
}

// cache time to 100us intervals to reduce system clock calls (it's noticeably slow if you sprinkle Timing::getTimeReal calls everywhere)
struct AutoTimeCache
{
	std::atomic<uint64_t> cachedTime{0};
	std::atomic<uint64_t> cacheTimestamp{0};

	// 100us is probably good enough to be basically realtime
	static constexpr uint64_t CACHE_DURATION_NS = 100 * NS_PER_US;
};

inline thread_local AutoTimeCache g_timeCache;

inline uint64_t getAutoTicksNS() noexcept
{
	const uint64_t now = getRealTicksNS();
	const uint64_t cachedTime = g_timeCache.cachedTime.load(std::memory_order_relaxed);
	const uint64_t cacheTimestamp = g_timeCache.cacheTimestamp.load(std::memory_order_relaxed);

	// cached
	if (now - cacheTimestamp < AutoTimeCache::CACHE_DURATION_NS && cachedTime != 0) [[likely]]
		return cachedTime;

	// uncached
	g_timeCache.cachedTime.store(now, std::memory_order_relaxed);
	g_timeCache.cacheTimestamp.store(now, std::memory_order_relaxed);

	return now;
}

} // namespace detail

// get nanoseconds since first timing call (globally) (equivalent to SDL_GetTicksNS)
inline uint64_t getTicksNS() noexcept
{
	return detail::getAutoTicksNS();
}

// get nanoseconds since first timing call (uncached, shouldn't ever need this)
inline uint64_t getLiveTicksNS() noexcept
{
	return detail::getRealTicksNS();
}

constexpr uint64_t ticksNSToMS(uint64_t ns) noexcept
{
	return detail::convertTime<NS_PER_MS>(ns);
}

inline uint64_t getTicksMS() noexcept
{
	return ticksNSToMS(getTicksNS());
}

inline uint64_t getLiveTicksMS() noexcept
{
	return ticksNSToMS(getLiveTicksNS());
}

// inspired by SDL_DelayPrecise
// basically sleep in small increments until we get close to the deadline, then spin
inline void sleepPrecise(uint64_t ns) noexcept
{
#ifdef MCENGINE_PLATFORM_WASM
	SDL_DelayPrecise(ns);
#else
	if (ns == 0)
	{
		detail::schedyield();
		return;
	}

	const uint64_t target_time = getLiveTicksNS() + ns;

	// adaptive thresholds based on total sleep duration
	constexpr uint64_t SPIN_THRESHOLD = 100 * NS_PER_US;  // 100us - below this, just spin
	constexpr uint64_t FINE_THRESHOLD = 2 * NS_PER_MS;    // 2ms - fine-grained sleep boundary
	constexpr uint64_t COARSE_THRESHOLD = 10 * NS_PER_MS; // 10ms - coarse sleep boundary

	// just spin for really short sleeps
	if (ns < SPIN_THRESHOLD) [[unlikely]]
	{
		uint64_t current_time = getLiveTicksNS();
		while (current_time < target_time)
		{
			detail::spinyield();
			current_time = getLiveTicksNS();
		}
		return;
	}

	uint64_t remaining_ns = ns;

	// for longer sleeps, use larger initial sleep chunk
	if (ns > COARSE_THRESHOLD)
	{
		// sleep for 90% of the total duration in one go for very long sleeps
		// only start doing smaller sleeps when we're pretty close to the target
		const uint64_t initial_sleep_ratio = ns > 100 * NS_PER_MS ? 95 : 90;
		const uint64_t initial_sleep = (ns * initial_sleep_ratio) / 100;
		std::this_thread::sleep_for(std::chrono::nanoseconds(initial_sleep));

		remaining_ns = target_time - getLiveTicksNS();
		if (remaining_ns > NS_PER_SECOND)
		{
			// clock adjustment or some bs, bail out
			return;
		}
	}
	else if (ns > FINE_THRESHOLD)
	{
		// sleep for most of the duration for med sleep
		const uint64_t initial_sleep = ns - FINE_THRESHOLD;
		std::this_thread::sleep_for(std::chrono::nanoseconds(initial_sleep));
		remaining_ns = FINE_THRESHOLD;
	}

	// fine-grained sleep phase (repeated small sleeps then spinwait the rest)
	uint64_t current_time = getLiveTicksNS();
	while (remaining_ns > SPIN_THRESHOLD && current_time < target_time)
	{
		// dynamically adjust sleep granularity based on remaining time
		uint64_t sleep_chunk;
		if (remaining_ns > 5 * NS_PER_MS)
			sleep_chunk = 2 * NS_PER_MS; // 2ms chunks for >5ms remaining
		else if (remaining_ns > NS_PER_MS)
			sleep_chunk = NS_PER_MS; // 1ms chunks for 1-5ms remaining (windows timer resolution is limited to 1ms)
		else
			sleep_chunk = remaining_ns / 2; // half of remaining for <1ms

		std::this_thread::sleep_for(std::chrono::nanoseconds(sleep_chunk));
		current_time = getLiveTicksNS();
		remaining_ns = target_time > current_time ? target_time - current_time : 0;
	}

	// spin until we're done
	while (current_time < target_time)
	{
		detail::spinyield();
		current_time = getLiveTicksNS();
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

// current time (since init.) in seconds as float (cached)
template <typename T = double>
    requires(std::floating_point<T>)
inline T getTimeReal() noexcept
{
	return timeNSToSeconds<T>(getTicksNS());
}

// current time (since init.) in seconds as float (uncached)
template <typename T = double>
    requires(std::floating_point<T>)
inline T getLiveTimeReal() noexcept
{
	return timeNSToSeconds<T>(getLiveTicksNS());
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

	inline void updateLive() noexcept
	{
		const uint64_t now = getLiveTicksNS();
		m_deltaSeconds = timeNSToSeconds<double>(now - m_lastUpdateNS);
		m_lastUpdateNS = now;
	}

	[[nodiscard]] constexpr double getDelta() const noexcept { return m_deltaSeconds; }

	[[nodiscard]] inline double getElapsedTime() const noexcept { return timeNSToSeconds<double>(m_lastUpdateNS - m_startTimeNS); }

	[[nodiscard]] inline uint64_t getElapsedTimeMS() const noexcept { return ticksNSToMS(m_lastUpdateNS - m_startTimeNS); }

	[[nodiscard]] inline uint64_t getElapsedTimeNS() const noexcept { return m_lastUpdateNS - m_startTimeNS; }

	// get elapsed time without needing update()
	[[nodiscard]] inline double getLiveElapsedTime() const noexcept { return timeNSToSeconds<double>(getLiveTicksNS() - m_startTimeNS); }

	[[nodiscard]] inline uint64_t getLiveElapsedTimeNS() const noexcept { return getLiveTicksNS() - m_startTimeNS; }

private:
	uint64_t m_startTimeNS{};
	uint64_t m_lastUpdateNS{};
	double m_deltaSeconds{};
};

}; // namespace Timing

using Timer = Timing::Timer;

#endif
