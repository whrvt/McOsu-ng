//========== Copyright (c) 2015, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		stopwatch/timer
//
// $NoKeywords: $time $sdltime
//===============================================================================//

#pragma once
#ifndef TIMER_H
#define TIMER_H

#include <SDL3/SDL.h>
#include <concepts>

class Timer
{
public:
	inline Timer(bool startOnCtor = true) {if (startOnCtor) start();}
	~Timer() = default;

	inline void start()
	{
		m_startTimeNS = SDL_GetTicksNS();
		m_currentTimeNS = m_startTimeNS;
		m_delta = 0.0;
		m_elapsedTime = 0.0;
		m_elapsedTimeMS = 0;
	}

	inline void update()
	{
		const uint64_t now = SDL_GetTicksNS();
		m_delta = static_cast<double>(now - m_currentTimeNS) / static_cast<double>(SDL_NS_PER_SECOND);
		const uint64_t elapsed = now - m_startTimeNS;
		m_elapsedTime = static_cast<double>(elapsed) / static_cast<double>(SDL_NS_PER_SECOND);
		m_elapsedTimeMS = elapsed / SDL_NS_PER_MS;
		m_currentTimeNS = now;
	}

	[[nodiscard]] inline double getDelta() const { return m_delta; }
	[[nodiscard]] inline double getElapsedTime() const { return m_elapsedTime; }
	[[nodiscard]] inline uint64_t getElapsedTimeMS() const { return m_elapsedTimeMS; }
private:
	uint64_t m_startTimeNS{};
	uint64_t m_currentTimeNS{};

	double m_delta{};
	double m_elapsedTime{};
	uint64_t m_elapsedTimeMS{};
};

namespace Timing {
	static inline void sleep(unsigned int us) {!!us ? SDL_DelayPrecise(static_cast<uint64_t>(us) * 1000) : SDL_Delay(0);}
	static inline void sleepNS(uint64_t ns) {!!ns ? SDL_DelayPrecise(ns) : SDL_Delay(0);}
	// seconds as a double
	template <typename T = double>
		requires (std::is_same_v<T, double> || std::convertible_to<T, double>)
	static inline T getTimeReal() {return static_cast<T>(SDL_GetTicksNS()) / static_cast<T>(SDL_NS_PER_SECOND);}
};

#endif
