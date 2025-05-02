//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		opengl explicit synchronization implementation
//
// $NoKeywords: $gls
//===============================================================================//

#include "OpenGLSync.h"
#if defined(MCENGINE_FEATURE_OPENGL) || defined(MCENGINE_FEATURE_GLES32)

#include "ConVar.h"
#include "Engine.h"

static constexpr auto DEFAULT_SYNC_TIMEOUT_US = 5000000; // 5ms timeout for sync operations
static constexpr auto DEFAULT_MAX_FRAMES_IN_FLIGHT = 2;

ConVar r_sync_timeout("r_sync_timeout", DEFAULT_SYNC_TIMEOUT_US, FCVAR_NONE, "timeout in microseconds for GPU synchronization operations");
ConVar r_sync_enabled("r_sync_enabled", true, FCVAR_NONE, "enable explicit GPU synchronization for OpenGL");
ConVar r_sync_debug("r_sync_debug", false, FCVAR_NONE, "print debug information about sync objects");
ConVar r_sync_max_frames("r_sync_max_frames", DEFAULT_MAX_FRAMES_IN_FLIGHT, FCVAR_NONE, "maximum pre-rendered frames allowed in rendering pipeline"); // (a la "Max Prerendered Frames")

OpenGLSync::OpenGLSync()
{
	m_iSyncFrameCount = 0;

	m_iMaxFramesInFlight = r_sync_max_frames.getVal<unsigned int>();
	r_sync_max_frames.setCallback( fastdelegate::MakeDelegate(this, &OpenGLSync::onFramecountNumChanged) );

	m_bEnabled = r_sync_enabled.getBool();
	r_sync_enabled.setCallback( fastdelegate::MakeDelegate(this, &OpenGLSync::onSyncBehaviorChanged) );
}

OpenGLSync::~OpenGLSync()
{
	while (!m_frameSyncQueue.empty())
	{
		deleteSyncObject(m_frameSyncQueue.front().syncObject);
		m_frameSyncQueue.pop_front();
	}
}

void OpenGLSync::begin()
{
	if (m_bEnabled)
		manageFrameSyncQueue(); // this will block and wait if we have too many frames in flight already
}

void OpenGLSync::end()
{
	if (m_bEnabled)
	{
		// create a new fence sync, to be signaled when current rendering commands complete on the GPU
		GLsync syncObj = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

		if (syncObj)
		{
			// add it to the queue
			FrameSyncPoint syncPoint;
			syncPoint.syncObject = syncObj;
			syncPoint.frameNumber = m_iSyncFrameCount++;

			m_frameSyncQueue.push_back(syncPoint);

			if (r_sync_debug.getBool())
				debugLog("Created sync object for frame %d (frames in flight: %d/%d)\n", syncPoint.frameNumber, m_frameSyncQueue.size(), m_iMaxFramesInFlight);
		}
	}
}

void OpenGLSync::manageFrameSyncQueue(bool forceWait)
{
	if (m_frameSyncQueue.empty())
		return;

	const bool debug = r_sync_debug.getBool();

	// check front of queue to see if it's complete
	bool needToWait = forceWait || (m_frameSyncQueue.size() >= m_iMaxFramesInFlight);

	// first, try a non-blocking check on the oldest sync object
	FrameSyncPoint &oldestSync = m_frameSyncQueue.front();
	GLint signaled = 0;

	GLsync sync = oldestSync.syncObject;
	glGetSynciv(sync, GL_SYNC_STATUS, sizeof(GLint), NULL, &signaled);

	// if oldest frame is done, we can remove it (and any other completed frames)
	if (signaled == GL_SIGNALED)
	{
		if (debug)
			debugLog("Frame %d already completed (no wait needed)\n", oldestSync.frameNumber);

		deleteSyncObject(oldestSync.syncObject);
		m_frameSyncQueue.pop_front();

		// check if more frames are done
		while (!m_frameSyncQueue.empty())
		{
			FrameSyncPoint &nextSync = m_frameSyncQueue.front();
			signaled = 0;

			sync = nextSync.syncObject;
			glGetSynciv(sync, GL_SYNC_STATUS, sizeof(GLint), NULL, &signaled);

			if (signaled == GL_SIGNALED)
			{
				if (debug)
					debugLog("Frame %d also completed\n", nextSync.frameNumber);

				deleteSyncObject(nextSync.syncObject);
				m_frameSyncQueue.pop_front();
			}
			else
			{
				break;
			}
		}
	}
	// if we need to wait and the frame isn't done yet, do a blocking wait with timeout
	else if (needToWait)
	{
		if (debug)
			debugLog("Waiting for frame %d to complete (frames in flight: %d/%d)\n", oldestSync.frameNumber, m_frameSyncQueue.size(), m_iMaxFramesInFlight);

		SYNC_RESULT result = waitForSyncObject(oldestSync.syncObject, r_sync_timeout.getInt());

		if (debug)
		{
			switch (result)
			{
			case SYNC_OBJECT_NOT_READY:
				debugLog("Frame %d sync object was not ready\n", oldestSync.frameNumber);
				break;
			case SYNC_ALREADY_SIGNALED:
				debugLog("Frame %d sync object was already signaled\n", oldestSync.frameNumber);
				break;
			case SYNC_GPU_COMPLETED:
				debugLog("Frame %d sync object was just completed\n", oldestSync.frameNumber);
				break;
			case SYNC_TIMEOUT_EXPIRED:
				debugLog("Frame %d sync object timed out after %d microseconds\n", oldestSync.frameNumber, r_sync_timeout.getInt());
				break;
			case SYNC_WAIT_FAILED:
				debugLog("Frame %d sync wait failed\n", oldestSync.frameNumber);
				break;
			}
		}

		// always remove the sync object we waited for, even if it timed out (don't get stuck if the GPU falls far behind)
		deleteSyncObject(oldestSync.syncObject);
		m_frameSyncQueue.pop_front();

		// check if other frames are done too
		if (result == SYNC_ALREADY_SIGNALED || result == SYNC_GPU_COMPLETED)
		{
			while (!m_frameSyncQueue.empty())
			{
				FrameSyncPoint &nextSync = m_frameSyncQueue.front();
				signaled = 0;

				sync = nextSync.syncObject;
				glGetSynciv(sync, GL_SYNC_STATUS, sizeof(GLint), NULL, &signaled);

				if (signaled == GL_SIGNALED)
				{
					if (debug)
						debugLog("Frame %d also completed\n", nextSync.frameNumber);

					deleteSyncObject(nextSync.syncObject);
					m_frameSyncQueue.pop_front();
				}
				else
				{
					break;
				}
			}
		}
	}
}

// wait for a sync object with timeout
OpenGLSync::SYNC_RESULT OpenGLSync::waitForSyncObject(GLsync syncObject, uint64_t timeoutNs)
{
	if (!syncObject)
		return SYNC_OBJECT_NOT_READY;

	GLsync sync = syncObject;

	// do a non-blocking check if already signaled
	GLint signaled = 0;
	glGetSynciv(sync, GL_SYNC_STATUS, sizeof(GLint), NULL, &signaled);

	if (signaled == GL_SIGNALED)
		return SYNC_ALREADY_SIGNALED;

	// wait with timeout
	GLenum result = glClientWaitSync(sync, GL_SYNC_FLUSH_COMMANDS_BIT, timeoutNs);

	switch (result)
	{
	case GL_ALREADY_SIGNALED:
		return SYNC_ALREADY_SIGNALED;
	case GL_TIMEOUT_EXPIRED:
		return SYNC_TIMEOUT_EXPIRED;
	case GL_WAIT_FAILED:
		return SYNC_WAIT_FAILED;
	case GL_CONDITION_SATISFIED:
		return SYNC_GPU_COMPLETED;
	default:
		return SYNC_WAIT_FAILED;
	}
}

void OpenGLSync::deleteSyncObject(GLsync syncObject)
{
	if (syncObject)
		glDeleteSync(syncObject);
}

void OpenGLSync::setMaxFramesInFlight(int maxFrames)
{
	m_iMaxFramesInFlight = maxFrames;

	// may need to wait on some frames if the limit is being reduced
	if (m_bEnabled && m_frameSyncQueue.size() > m_iMaxFramesInFlight)
	{
		if (r_sync_debug.getBool())
			debugLog("Max frames reduced to %d, waiting for excess frames\n", m_iMaxFramesInFlight);
		// wait
		manageFrameSyncQueue(true);
	}
}

#endif