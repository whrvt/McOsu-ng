//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		fps timer
//
// $NoKeywords: $sdltime $os
//===============================================================================//

#pragma once
#ifndef SDLTIMER_H
#define SDLTIMER_H

#include "Timer.h"
#include <SDL3/SDL.h>

class SDLTimer : public BaseTimer
{
public:
	SDLTimer();
	virtual ~SDLTimer() {;}

	virtual void start() override;
	virtual void update() override;

	virtual inline double getDelta() const override {return m_delta;}
	virtual inline double getElapsedTime() const override {return m_elapsedTime;}
	virtual inline uint64_t getElapsedTimeMS() const override {return m_elapsedTimeMS;}

private:
	uint64_t m_startTimeNS;
	uint64_t m_currentTimeNS;

	double m_delta;
	double m_elapsedTime;
	uint64_t m_elapsedTimeMS;
};

#endif
