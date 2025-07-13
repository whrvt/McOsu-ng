//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		frame limiter, to be called at the end of a main loop interation
//
// $NoKeywords: $fps $limiter
//===============================================================================//

#include "FPSLimiter.h"
#include "Timing.h"
#include "ConVar.h"

unsigned long long FPSLimiter::s_iNextFrameTime{0};

void FPSLimiter::limitFrames(unsigned int targetFPS)
{
	if (targetFPS > 0)
	{
		const uint64_t frameTimeNS = Timing::NS_PER_SECOND / static_cast<uint64_t>(targetFPS);
		const uint64_t now = Timing::getTicksNS();

		// if we're ahead of schedule, sleep until next frame
		if (s_iNextFrameTime > now)
		{
			const uint64_t sleepTime = s_iNextFrameTime - now;
			Timing::sleepNS(sleepTime);
		}
		else
		{
			// behind schedule or exactly on time, reset to now
			s_iNextFrameTime = now;
		}
		// set time for next frame
		s_iNextFrameTime += frameTimeNS;
	}
	if (cv::fps_yield.getBool())
		Timing::sleep(0);
}
