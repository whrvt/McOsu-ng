//================ Copyright (c) 2019, PG, All rights reserved. =================//
//
// Purpose:		fps timer
//
// $NoKeywords: $nxtime $os
//===============================================================================//

#pragma once
#ifndef HORIZONTIMER_H
#define HORIZONTIMER_H

#include "Timer.h"

#ifdef __SWITCH__

class HorizonTimer : public BaseTimer
{
public:
	HorizonTimer(bool startOnCtor = true) {if (startOnCtor) start();}
	~HorizonTimer() override = default;

	void start() override;
	void update() override;

	[[nodiscard]] inline double getDelta() const override { return m_delta; }
	[[nodiscard]] inline double getElapsedTime() const override { return m_elapsedTime; }
	[[nodiscard]] inline uint64_t getElapsedTimeMS() const override { return m_elapsedTimeMS; }

private:
    uint64_t m_startTime{};
    uint64_t m_currentTime{};

    double m_delta{};
    double m_elapsedTime{};
    uint64_t m_elapsedTimeMS{};
};

#else
using HorizonTimer = DummyTimer;
#endif

#endif
