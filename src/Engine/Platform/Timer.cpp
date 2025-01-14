//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		fps timer
//
// $NoKeywords: $time $os
//===============================================================================//

#include "Timer.h"

#if defined(_WIN32) || defined(_WIN64) || defined(SDL_PLATFORM_WIN32) || defined(__CYGWIN__) || defined(__CYGWIN32__) || defined(__TOS_WIN__) || defined(SDL_PLATFORM_WINDOWS)

#include "WinTimer.h"

#elif defined __linux__

#include "LinuxTimer.h"

#elif defined SDL_PLATFORM_APPLE

#include "MacOSTimer.h"

#elif defined __SWITCH__

#include "HorizonTimer.h"

#endif

Timer::Timer()
{
	m_timer = NULL;

#if defined(_WIN32) || defined(_WIN64) || defined(SDL_PLATFORM_WIN32) || defined(__CYGWIN__) || defined(__CYGWIN32__) || defined(__TOS_WIN__) || defined(SDL_PLATFORM_WINDOWS)

	m_timer = new WinTimer();

#elif defined __linux__

	m_timer = new LinuxTimer();

#elif defined SDL_PLATFORM_APPLE

	m_timer = new MacOSTimer();

#elif defined __SWITCH__

	m_timer = new HorizonTimer();

#else

#error Missing Timer implementation for OS!

#endif

}

Timer::~Timer()
{
	SAFE_DELETE(m_timer);
}
