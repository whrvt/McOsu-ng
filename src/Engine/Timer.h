//========== Copyright (c) 2015, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		fps timer
//
// $NoKeywords: $time
//===============================================================================//

#pragma once
#ifndef TIMER_H
#define TIMER_H

#include "config.h"
#include <cstdint>

class BaseTimer
{
public:
	BaseTimer() = default;
	virtual ~BaseTimer() = default;

	virtual void start() = 0;
	virtual void update() = 0;

	[[nodiscard]] virtual inline double getDelta() const = 0;
	[[nodiscard]] virtual inline double getElapsedTime() const = 0;
	[[nodiscard]] virtual inline uint64_t getElapsedTimeMS() const = 0;
};

#include "SDLTimer.h"

#endif
