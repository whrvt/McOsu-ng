//================ Copyright (c) 2015, PG, All rights reserved. =================//
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

class DummyTimer : public BaseTimer
{
public:
	DummyTimer(bool _){};
	~DummyTimer() = default;
	void start() {}
	void update() {}
	[[nodiscard]] inline double getDelta() const {return 0.0;}
	[[nodiscard]] inline double getElapsedTime() const {return 0.0;}
	[[nodiscard]] inline uint64_t getElapsedTimeMS() const {return 0;}
};

class Timer
{
public:
	Timer(bool startOnCtor = true);
	~Timer() { delete m_timer; }

	inline void start() { m_timer->start(); }
	inline void update() { m_timer->update(); }

	[[nodiscard]] inline double getDelta() const { return m_timer->getDelta(); }
	[[nodiscard]] inline double getElapsedTime() const { return m_timer->getElapsedTime(); }
	[[nodiscard]] inline uint64_t getElapsedTimeMS() const { return m_timer->getElapsedTimeMS(); }

private:
	BaseTimer *m_timer;
};

#endif
