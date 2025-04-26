//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		fps timer
//
// $NoKeywords: $linuxtime $os
//===============================================================================//

#pragma once
#ifndef LINUXTIMER_H
#define LINUXTIMER_H

#include "Timer.h"

#if defined(__linux__) && !defined(MCENGINE_FEATURE_SDL)

#include <time.h>

class LinuxTimer : public BaseTimer
{
public:
	LinuxTimer(bool startOnCtor = true) {if (startOnCtor) start();}
	~LinuxTimer() override = default;

	void start() override;
	void update() override;

	[[nodiscard]] inline double getDelta() const override { return m_delta; }
	[[nodiscard]] inline double getElapsedTime() const override { return m_elapsedTime; }
	[[nodiscard]] inline uint64_t getElapsedTimeMS() const override { return m_elapsedTimeMS; }

private:
	timespec m_startTime{};
	timespec m_currentTime{};

	double m_delta{};
	double m_elapsedTime{};
	uint64_t m_elapsedTimeMS{};
};

#else
using LinuxTimer = DummyTimer;
#endif

#endif
