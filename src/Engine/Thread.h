//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		thread wrapper
//
// $NoKeywords: $thread
//===============================================================================//

#pragma once
#ifndef THREAD_H
#define THREAD_H

#include "Engine.h" // for debugLog
#include <functional>
#include <stop_token>
#include <thread>

class McThread final
{
public:
	

	typedef void *(*START_ROUTINE)(void *);
	typedef void *(*START_ROUTINE_WITH_STOP_TOKEN)(void *, std::stop_token);

public:
	// backward compatibility
	McThread(START_ROUTINE start_routine, void *arg)
#ifndef __EXCEPTIONS
	    noexcept
#endif
	{
		m_bReady = false;
		m_bUsesStopToken = false;
#ifdef __EXCEPTIONS
		try
		{
#endif
			// wrap the legacy function for jthreads
			m_thread = std::jthread([start_routine, arg](std::stop_token) -> void { start_routine(arg); });
			m_bReady = true;
#ifdef __EXCEPTIONS
		}
		catch (const std::system_error &e)
		{
			if (cv::debug_thread.getBool())
				debugLog("McThread Error: std::jthread constructor exception: {:s}\n", e.what());
		}
#endif
	}

	// constructor which takes a stop token directly
	McThread(START_ROUTINE_WITH_STOP_TOKEN start_routine, void *arg)
#ifndef __EXCEPTIONS
	    noexcept
#endif
	{
		m_bReady = false;
		m_bUsesStopToken = true;
#ifdef __EXCEPTIONS
		try
		{
#endif
			m_thread = std::jthread([start_routine, arg](std::stop_token token) -> void { start_routine(arg, token); });
			m_bReady = true;
#ifdef __EXCEPTIONS
		}
		catch (const std::system_error &e)
		{
			if (cv::debug_thread.getBool())
				debugLog("McThread Error: std::jthread constructor exception: {:s}\n", e.what());
		}
#endif
	}

	// ctor that takes a std::function directly
	McThread(std::function<void(std::stop_token)> func)
#ifndef __EXCEPTIONS
	    noexcept
#endif
	{
		m_bReady = false;
		m_bUsesStopToken = true;
#ifdef __EXCEPTIONS
		try
		{
#endif
			m_thread = std::jthread(std::move(func));
			m_bReady = true;
#ifdef __EXCEPTIONS
		}
		catch (const std::system_error &e)
		{
			if (cv::debug_thread.getBool())
				debugLog("McThread Error: std::jthread constructor exception: {:s}\n", e.what());
		}
#endif
	}

	~McThread()
	{
		if (!m_bReady)
			return;

		m_bReady = false;

		// jthread automatically requests stop and joins in destructor
		// but we can be explicit about it
		if (m_thread.joinable())
		{
			m_thread.request_stop();
			m_thread.join();
		}
	}

	[[nodiscard]] inline bool isReady() const { return m_bReady; }
	[[nodiscard]] inline bool isStopRequested() const { return m_bReady && m_thread.get_stop_token().stop_requested(); }

	inline void requestStop()
	{
		if (m_bReady)
			m_thread.request_stop();
	}

	[[nodiscard]] inline std::stop_token getStopToken() const { return m_thread.get_stop_token(); }
	[[nodiscard]] inline bool usesStopToken() const { return m_bUsesStopToken; }

private:
	std::jthread m_thread;
	bool m_bReady;
	bool m_bUsesStopToken;
};

#endif
