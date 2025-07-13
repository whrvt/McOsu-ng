//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		frame limiter, to be called at the end of a main loop interation
//
// $NoKeywords: $fps $limiter
//===============================================================================//

#pragma once

class FPSLimiter final
{
public:
	FPSLimiter() = delete;
	~FPSLimiter() = delete;
	FPSLimiter &operator=(const FPSLimiter &) = delete;
	FPSLimiter &operator=(FPSLimiter &&) = delete;
	FPSLimiter(const FPSLimiter &) = delete;
	FPSLimiter(FPSLimiter &&) = delete;

	static void limitFrames(unsigned int targetFPS);
	static inline void reset() { s_iNextFrameTime = 0; }

private:
	static unsigned long long s_iNextFrameTime;
};
