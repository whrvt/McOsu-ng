//================ Copyright (c) 2020, PG, All rights reserved. =================//
//
// Purpose:		thread wrapper
//
// $NoKeywords: $thread $os
//===============================================================================//

#include "Thread.h"
#include "ConVar.h"
#include "Engine.h"

#include "HorizonThread.h"

#if defined(MCENGINE_FEATURE_MULTITHREADING) && !defined(__SWITCH__)
#include <thread>

// std::thread implementation of Thread
class StdThread : public BaseThread
{
public:
	StdThread(McThread::START_ROUTINE start_routine, void *arg) : BaseThread()
	{
		m_bReady = false;

		try
		{
			m_thread = std::thread(start_routine, arg);
			m_bReady = true;
		}
		catch (const std::system_error& e)
		{
			if (McThread::debug->getBool())
				debugLog("StdThread Error: std::thread constructor exception: %s\n", e.what());
		}
	}

	virtual ~StdThread()
	{
		if (!m_bReady) return;

		m_bReady = false;

		if (m_thread.joinable())
			m_thread.join();
	}

	bool isReady()
	{
		return m_bReady;
	}

private:
	std::thread m_thread;
	bool m_bReady;
};

#else
class StdThread : public BaseThread{};
#endif // defined(MCENGINE_FEATURE_MULTITHREADING) && !defined(__SWITCH__)

ConVar debug_thread("debug_thread", false, FCVAR_NONE);

ConVar *McThread::debug = &debug_thread;

McThread::McThread(START_ROUTINE start_routine, void *arg)
{
#ifdef MCENGINE_FEATURE_MULTITHREADING

#ifdef __SWITCH__
	m_baseThread = new HorizonThread(start_routine, arg);
#else
	m_baseThread = new StdThread(start_routine, arg);
#endif

#else
	m_baseThread = NULL;
#endif // MCENGINE_FEATURE_MULTITHREADING
}

McThread::~McThread()
{
	SAFE_DELETE(m_baseThread);
}

bool McThread::isReady()
{
	return (m_baseThread != NULL && m_baseThread->isReady());
}
