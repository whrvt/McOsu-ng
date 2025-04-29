//================ Copyright (c) 2020, PG, All rights reserved. =================//
//
// Purpose:		thread wrapper
//
// $NoKeywords: $thread
//===============================================================================//

#pragma once
#ifndef THREAD_H
#define THREAD_H

class ConVar;

class BaseThread
{
public:
	virtual ~BaseThread() {;}

	[[nodiscard]] virtual bool isReady() const = 0;
};

class McThread
{
public:
	static ConVar *debug;

	typedef void *(*START_ROUTINE)(void*);

public:
	McThread(START_ROUTINE start_routine, void *arg);
	~McThread() {if (m_baseThread) { delete (m_baseThread); (m_baseThread) = nullptr; }}

	[[nodiscard]] inline bool isReady() const {return m_baseThread != nullptr && m_baseThread->isReady();};

private:
	BaseThread *m_baseThread;
};

#endif
