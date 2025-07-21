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

#include <SDL3/SDL_timer.h>

#include <thread>
#include <concepts>
#include <cstdint>
#include <execution>

#define MC_STRINGIZE(x) #x
#if defined(_WIN32) && !defined(_DEBUG)
#define MC_PRINT(...) printf("%s", fmt::format(__VA_ARGS__).c_str())
#else
#define MC_PRINT(...) fmt::print(__VA_ARGS__)
#endif
#define DBGTIME(__amt__, ...) \
	do \
	{ \
		static_assert((uint32_t)__amt__ > 0 && (uint32_t)__amt__ <= 4096); \
		static std::array<double, (uint32_t)__amt__> lasttms__{}; \
		static uint32_t lti__ = 0; \
		static double overall_max__ = 0.0; \
		const double before__ = Timing::getTimeReal(); \
		do \
		{ \
			__VA_ARGS__; \
		} while (false); \
		const double after__ = Timing::getTimeReal(); \
		lasttms__[lti__ % (uint32_t)__amt__] = after__ - before__; \
		if (!(++lti__ % (uint32_t)__amt__)) \
		{ \
			lti__ = 0; \
			auto current_max__ = std::ranges::max(lasttms__); \
			if (current_max__ > overall_max__) \
				overall_max__ = current_max__; \
			MC_PRINT("\n\tTIME FOR: " #__VA_ARGS__ "\n\tmax overall: {:.8f}" \
			         "\n\taverage: {:.4f} min: {:.4f} max: {:.4f}" \
			         "\n\tpast " MC_STRINGIZE(__amt__) " times:" \
			                                           "\n\t[ {:.4f} ]\n", \
			         overall_max__, std::reduce(std::execution::unseq, lasttms__.begin(), lasttms__.end(), 0.0) / ((uint32_t)__amt__), std::ranges::min(lasttms__), \
			         current_max__, fmt::join(lasttms__, ", ")); \
		} \
	} while (false);

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
forceinline void yield_internal() noexcept
{
	if constexpr (Env::cfg(OS::WASM))
		SDL_Delay(0);
	else
		std::this_thread::yield();
}

template <uint64_t Ratio>
constexpr uint64_t convertTime(uint64_t ns) noexcept
{
	return ns / Ratio;
}

} // namespace detail

inline INLINE_BODY uint64_t getTicksNS() noexcept
{
	return SDL_GetTicksNS();
}

constexpr uint64_t ticksNSToMS(uint64_t ns) noexcept
{
	return detail::convertTime<NS_PER_MS>(ns);
}

inline uint64_t getTicksMS() noexcept
{
	return ticksNSToMS(getTicksNS());
}

inline INLINE_BODY void sleepPrecise(uint64_t ns) noexcept
{
	SDL_DelayPrecise(ns);
}

inline void sleep(uint64_t us) noexcept
{
	!!us ? sleepPrecise(us * NS_PER_US) : detail::yield_internal();
}

inline void sleepNS(uint64_t ns) noexcept
{
	!!ns ? sleepPrecise(ns) : detail::yield_internal();
}

inline void sleepMS(uint64_t ms) noexcept
{
	!!ms ? sleepPrecise(ms * NS_PER_MS) : detail::yield_internal();
}

template <typename T = double>
    requires(std::floating_point<T>)
constexpr T timeNSToSeconds(uint64_t ns) noexcept
{
	return static_cast<T>(ns) / static_cast<T>(NS_PER_SECOND);
}

// current time (since init.) in seconds as float
// decoupled from engine updates!
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
