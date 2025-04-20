//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		fps timer
//
// $NoKeywords: $time $os
//===============================================================================//

#include "Timer.h"

#ifdef MCENGINE_FEATURE_SDL

#include "SDLTimer.h"

#elif defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__CYGWIN__) || defined(__CYGWIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__)

#include "WinTimer.h"

#elif defined __linux__

#include "LinuxTimer.h"

#elif defined __APPLE__

#include "MacOSTimer.h"

#elif defined __SWITCH__

#include "HorizonTimer.h"

#endif

Timer::Timer()
{
	m_timer = NULL;

#ifdef MCENGINE_FEATURE_SDL

	m_timer = new SDLTimer();

#elif defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__CYGWIN__) || defined(__CYGWIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__)

	m_timer = new WinTimer();

#elif defined __linux__

	m_timer = new LinuxTimer();

#elif defined __APPLE__

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
