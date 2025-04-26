//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		fps timer
//
// $NoKeywords: $time $os
//===============================================================================//

#include "Timer.h"
#include "SDLTimer.h"
#include "WinTimer.h"
#include "LinuxTimer.h"
#include "MacOSTimer.h"
#include "HorizonTimer.h"

#include "BaseEnvironment.h"

Timer::Timer(bool startOnCtor)
{
	m_timer = nullptr;
	if constexpr (Env::cfg(BACKEND::SDL))
		m_timer = new SDLTimer(startOnCtor);
	else if constexpr (Env::cfg(OS::WINDOWS))
		m_timer = new WinTimer(startOnCtor);
	else if constexpr (Env::cfg(OS::LINUX))
		m_timer = new LinuxTimer(startOnCtor);
	else if constexpr (Env::cfg(OS::MACOS))
		m_timer = new MacOSTimer(startOnCtor);
	else if constexpr (Env::cfg(OS::HORIZON))
		m_timer = new HorizonTimer(startOnCtor);

	static_assert(!Env::cfg(OS::NONE), "No timer implementation for this platform!");
}
