//================ Copyright (c) 2020, PG, All rights reserved. =================//
//
// Purpose:		thread wrapper
//
// $NoKeywords: $thread $os
//===============================================================================//

#include "Thread.h"
#include "ConVar.h"
#include "Engine.h"

#include <thread>

// std::thread implementation of Thread
class StdThread : public BaseThread
{
public:
#ifdef __EXCEPTIONS
	StdThread(McThread::START_ROUTINE start_routine, void *arg) : BaseThread()
#else
	StdThread(McThread::START_ROUTINE start_routine, void *arg) noexcept : BaseThread()
#endif
	{
		m_bReady = false;
#ifdef __EXCEPTIONS
		try
		{
#endif
			m_thread = std::thread(start_routine, arg);
			m_bReady = true;
#ifdef __EXCEPTIONS
		}
		catch (const std::system_error& e)
		{
			if (McThread::debug->getBool())
				debugLog("StdThread Error: std::thread constructor exception: {:s}\n", e.what());
		}
#endif
	}

	~StdThread() override
	{
		if (!m_bReady) return;

		m_bReady = false;

		if (m_thread.joinable())
			m_thread.join();
	}

	[[nodiscard]] inline bool isReady() const override {return m_bReady;}

private:
	std::thread m_thread;
	bool m_bReady;
};

ConVar debug_thread("debug_thread", false, FCVAR_NONE);

ConVar *McThread::debug = &debug_thread;

McThread::McThread(START_ROUTINE start_routine, void *arg)
{
	m_baseThread = new StdThread(start_routine, arg);
}

