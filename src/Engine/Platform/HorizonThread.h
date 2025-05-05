//================ Copyright (c) 2020, PG, All rights reserved. =================//
//
// Purpose:		horizon implementation of thread
//
// $NoKeywords: $nxthread $os
//===============================================================================//

#pragma once
#ifndef HORIZONTHREAD_H
#define HORIZONTHREAD_H

#include "Thread.h"

#ifdef __SWITCH__

#include <switch.h>

class HorizonThread : public BaseThread
{
public:
	HorizonThread(McThread::START_ROUTINE start_routine, void *arg);
	virtual ~HorizonThread();

	[[nodiscard]] inline bool isReady() const {return m_bReady;}

private:
	bool m_bReady;

	Thread m_thread;
};

#else
class HorizonThread : public BaseThread{};
#endif

#endif
