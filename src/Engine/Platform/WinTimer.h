//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		fps timer
//
// $NoKeywords: $wintime $os
//===============================================================================//

#pragma once
#ifndef WINTIMER_H
#define WINTIMER_H

#include "Timer.h"
#if !defined(MCENGINE_FEATURE_SDL) && defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__CYGWIN__) || defined(__CYGWIN32__) ||              \
    defined(__TOS_WIN__) || defined(__WINDOWS__)

// #define WIN32_LEAN_AND_MEAN
// #define NOCRYPT
#include <basetsd.h>
#include <windows.h>

class WinTimer : public BaseTimer
{
public:
	WinTimer(bool startOnCtor = true);
	~WinTimer() override = default;

	void start() override;
	void update() override;

	[[nodiscard]] inline double getDelta() const override { return m_delta; }
	[[nodiscard]] inline double getElapsedTime() const override { return m_elapsedTime; }
	[[nodiscard]] inline uint64_t getElapsedTimeMS() const override { return m_elapsedTimeMS; }

private:
	double m_secondsPerTick;
	LONGLONG m_ticksPerSecond;

	LARGE_INTEGER m_currentTime{};
	LARGE_INTEGER m_startTime{};

	double m_delta{};
	double m_elapsedTime{};
	uint64_t m_elapsedTimeMS{};
};

#else
using WinTimer = DummyTimer;
#endif

#endif
