//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		fps timer
//
// $NoKeywords: $wintime $os
//===============================================================================//

#if defined(_WIN32) || defined(_WIN64) || defined(SDL_PLATFORM_WIN32) || defined(__CYGWIN__) || defined(__CYGWIN32__) || defined(__TOS_WIN__) || defined(SDL_PLATFORM_WINDOWS)

#include "WinTimer.h"

WinTimer::WinTimer() : BaseTimer()
{
	LARGE_INTEGER ticks;
	QueryPerformanceFrequency(&ticks);

	m_secondsPerTick = 1.0 / (double)ticks.QuadPart;
	m_ticksPerSecond = ticks.QuadPart;

	m_startTime.QuadPart = 0;
	m_currentTime.QuadPart = 0;

	m_delta = 0.0;
	m_elapsedTime = 0.0;
	m_elapsedTimeMS = 0;
}

void WinTimer::start()
{
	QueryPerformanceCounter(&m_startTime);
	m_currentTime = m_startTime;

	m_delta = 0.0;
	m_elapsedTime = 0.0;
	m_elapsedTimeMS = 0;
}

void WinTimer::update()
{
	LARGE_INTEGER nowTime;
	QueryPerformanceCounter(&nowTime);

	m_delta = (double)(nowTime.QuadPart - m_currentTime.QuadPart) * m_secondsPerTick;
	m_elapsedTime = (double)(nowTime.QuadPart - m_startTime.QuadPart) * m_secondsPerTick;
	m_elapsedTimeMS = (uint64_t)(((nowTime.QuadPart - m_startTime.QuadPart) * 1000) / m_ticksPerSecond);
	m_currentTime = nowTime;
}

#endif
