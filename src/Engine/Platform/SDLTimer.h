//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		fps timer
//
// $NoKeywords: $sdltime $os
//===============================================================================//

#pragma once
#ifndef SDLTIMER_H
#define SDLTIMER_H

#include "Timer.h"

#include <SDL3/SDL.h>

class SDLTimer : public BaseTimer
{
public:
	inline SDLTimer(bool startOnCtor = true) {if (startOnCtor) start();}
	~SDLTimer() override = default;

	inline void start() override
	{
		m_startTimeNS = SDL_GetTicksNS();
		m_currentTimeNS = m_startTimeNS;
		m_delta = 0.0;
		m_elapsedTime = 0.0;
		m_elapsedTimeMS = 0;
	}

	inline void update() override
	{
		const uint64_t now = SDL_GetTicksNS();
		m_delta = static_cast<double>(now - m_currentTimeNS) / static_cast<double>(SDL_NS_PER_SECOND);
		const uint64_t elapsed = now - m_startTimeNS;
		m_elapsedTime = static_cast<double>(elapsed) / static_cast<double>(SDL_NS_PER_SECOND);
		m_elapsedTimeMS = elapsed / SDL_NS_PER_MS;
		m_currentTimeNS = now;
	}

	[[nodiscard]] inline double getDelta() const override { return m_delta; }
	[[nodiscard]] inline double getElapsedTime() const override { return m_elapsedTime; }
	[[nodiscard]] inline uint64_t getElapsedTimeMS() const override { return m_elapsedTimeMS; }

private:
	uint64_t m_startTimeNS{};
	uint64_t m_currentTimeNS{};

	double m_delta{};
	double m_elapsedTime{};
	uint64_t m_elapsedTimeMS{};
};

using Timer = SDLTimer;

#endif
