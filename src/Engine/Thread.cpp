//================ Copyright (c) 2020, PG, All rights reserved. =================//
//
// Purpose:		thread wrapper
//
// $NoKeywords: $thread $os
//===============================================================================//

#include "Thread.h"
#include "ConVar.h"
#include "Engine.h"

#if defined(MCENGINE_FEATURE_MULTITHREADING)
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

#else
class StdThread : public BaseThread{};
#endif // defined(MCENGINE_FEATURE_MULTITHREADING)

ConVar debug_thread("debug_thread", false, FCVAR_NONE);

ConVar *McThread::debug = &debug_thread;

McThread::McThread(START_ROUTINE start_routine, void *arg)
{
#ifdef MCENGINE_FEATURE_MULTITHREADING
	m_baseThread = new StdThread(start_routine, arg);
#else
	m_baseThread = NULL;
#endif // MCENGINE_FEATURE_MULTITHREADING
}

