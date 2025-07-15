//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		opengl explicit synchronization implementation
//
// $NoKeywords: $gls
//===============================================================================//

#pragma once
#ifndef OPENGLSYNC_H
#define OPENGLSYNC_H

#include "cbase.h"

#if defined(MCENGINE_FEATURE_OPENGL) || defined(MCENGINE_FEATURE_GLES32)
#include <deque>

typedef struct __GLsync *GLsync;

class OpenGLSync
{
public:
	OpenGLSync();
	~OpenGLSync();

	void begin(); // call at the beginning of beginScene()
	void end();   // call in endScene()

private:
	enum SYNC_RESULT
	{
		SYNC_OBJECT_NOT_READY, // sync object not created or already signaled
		SYNC_ALREADY_SIGNALED, // GPU already done with the work
		SYNC_TIMEOUT_EXPIRED,  // waited but timed out
		SYNC_WAIT_FAILED,      // error during waiting
		SYNC_GPU_COMPLETED     // GPU just completed the work during this wait
	};

	SYNC_RESULT waitForSyncObject(GLsync syncObject, uint64_t timeoutNs);                  // wait for a specific sync object to be reached
	void deleteSyncObject(GLsync syncObject);                                              // delete a sync object
	void setMaxFramesInFlight(int maxFrames);                                              // set maximum frames in flight (default: 2)
	[[nodiscard]] int getMaxFramesInFlight() const { return m_iMaxFramesInFlight; }        // get current maximum frames in flight
	[[nodiscard]] int getCurrentFramesInFlight() const { return m_frameSyncQueue.size(); } // get actual frames in flight
	void manageFrameSyncQueue(bool forceWait = false);                                     // Clean up expired sync objects and limit frames in flight

	typedef struct
	{
		GLsync syncObject;
		unsigned int frameNumber;
	} FrameSyncPoint;

	std::deque<FrameSyncPoint> m_frameSyncQueue; // queue of frame sync points
	int m_iMaxFramesInFlight;                    // maximum allowed frames in flight
	unsigned int m_iSyncFrameCount;              // counter for sync-related tracking
	bool m_bEnabled;                             // enabledness
	bool m_bAvailable;                           // if the opengl feature is even available at all

	// callbacks
	inline void onSyncBehaviorChanged(const UString & /*oldValue*/, const UString &newValue) { m_bEnabled = m_bAvailable && !!newValue.toInt(); }
	inline void onFramecountNumChanged(const UString & /*oldValue*/, const UString &newValue)
	{
		m_iMaxFramesInFlight = newValue.to<unsigned int>();
		if (m_iMaxFramesInFlight > 0)
			setMaxFramesInFlight(m_iMaxFramesInFlight);
	}
};

#endif
#endif