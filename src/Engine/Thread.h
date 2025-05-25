//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		thread wrapper
//
// $NoKeywords: $thread
//===============================================================================//

#pragma once
#ifndef THREAD_H
#define THREAD_H

#include "Engine.h"
#include <thread>

class McThread final
{
public:
	static ConVar *debug;

	typedef void *(*START_ROUTINE)(void*);

public:
#ifdef __EXCEPTIONS
	McThread(START_ROUTINE start_routine, void *arg)
#else
	McThread(START_ROUTINE start_routine, void *arg) noexcept
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
				debugLog("McThread Error: std::thread constructor exception: {:s}\n", e.what());
		}
#endif
	}

	~McThread()
	{
		if (!m_bReady) return;

		m_bReady = false;

		if (m_thread.joinable())
			m_thread.join();
	}

	[[nodiscard]] inline bool isReady() const {return m_bReady;}

private:
	std::thread m_thread;
	bool m_bReady;
};

#endif
