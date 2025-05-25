//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		thread wrapper
//
// $NoKeywords: $thread $os
//===============================================================================//

#include "Thread.h"

ConVar debug_thread("debug_thread", false, FCVAR_NONE);

ConVar *McThread::debug = &debug_thread;
