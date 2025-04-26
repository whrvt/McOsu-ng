//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		fps timer
//
// $NoKeywords: $mactime $os
//===============================================================================//


#pragma once
#ifndef MACOSTIMER_H
#define MACOSTIMER_H

#include "Timer.h"

#if defined(__APPLE__) && !defined(MCENGINE_FEATURE_SDL)

#include <mach/mach.h>
#include <mach/mach_time.h>

class MacOSTimer : public BaseTimer
{
public:
	inline MacOSTimer(bool startOnCtor = true);
	~MacOSTimer() override = default;

	void start() override;
	void update() override;

	[[nodiscard]] inline double getDelta() const override { return m_delta; }
	[[nodiscard]] inline double getElapsedTime() const override { return m_elapsedTime; }
	[[nodiscard]] inline uint64_t getElapsedTimeMS() const override { return m_elapsedTimeMS; }

private:
	mach_timebase_info_data_t m_timebaseInfo;
	uint64_t m_currentTime{};
	uint64_t m_startTime{};

	double m_delta{};
	double m_elapsedTime{};
	uint64_t m_elapsedTimeMS{};
};

using Timer = MacOSTimer;

#else
class MacOSTimer : public BaseTimer
{};
#endif

#endif
