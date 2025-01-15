//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		fps timer
//
// $NoKeywords: $sdltime $os
//===============================================================================//

#include "SDLTimer.h"

SDLTimer::SDLTimer() : BaseTimer()
{
	m_startTimeNS = 0;
	m_currentTimeNS = 0;

	m_delta = 0.0;
	m_elapsedTime = 0.0;
	m_elapsedTimeMS = 0;
}

void SDLTimer::start()
{
	m_startTimeNS = SDL_GetTicksNS();
	m_currentTimeNS = m_startTimeNS;

	m_delta = 0.0;
	m_elapsedTime = 0.0;
	m_elapsedTimeMS = 0;
}

void SDLTimer::update()
{
	const uint64_t now = SDL_GetTicksNS();

	m_delta = static_cast<double>(now - m_currentTimeNS) / static_cast<double>(SDL_NS_PER_SECOND);

	const uint64_t elapsed = now - m_startTimeNS;
	m_elapsedTime = static_cast<double>(elapsed) / static_cast<double>(SDL_NS_PER_SECOND);
	m_elapsedTimeMS = elapsed / SDL_NS_PER_MS;

	m_currentTimeNS = now;
}
