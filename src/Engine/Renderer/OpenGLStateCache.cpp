//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		OpenGL state cache for reducing synchronization points (WIP)
//
// $NoKeywords: $glsc
//===============================================================================//

#include "OpenGLStateCache.h"

#if defined(MCENGINE_FEATURE_OPENGL) || defined(MCENGINE_FEATURE_GLES2) || defined(MCENGINE_FEATURE_GLES32)

#include "ConVar.h"
#include "OpenGLHeaders.h"

OpenGLStateCache *OpenGLStateCache::s_instance = nullptr;

OpenGLStateCache::OpenGLStateCache()
{
	m_iCurrentProgram = 0;
	m_iCurrentFramebuffer = 0;
	m_iViewport[0] = 0;
	m_iViewport[1] = 0;
	m_iViewport[2] = 0;
	m_iViewport[3] = 0;
	m_bInitialized = false;
}

OpenGLStateCache &OpenGLStateCache::getInstance()
{
	if (s_instance == nullptr)
		s_instance = new OpenGLStateCache();

	return *s_instance;
}

void OpenGLStateCache::initialize()
{
	if (m_bInitialized)
		return;

	// one-time initialization of cache from actual GL state
	refresh();
	m_bInitialized = true;
}

void OpenGLStateCache::refresh()
{
	// only do the expensive query when necessary
	glGetIntegerv(GL_CURRENT_PROGRAM, &m_iCurrentProgram);
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &m_iCurrentFramebuffer);
	glGetIntegerv(GL_VIEWPORT, m_iViewport);
}

void OpenGLStateCache::setCurrentProgram(int program)
{
	m_iCurrentProgram = program;
}

int OpenGLStateCache::getCurrentProgram() const
{
	return m_iCurrentProgram;
}

void OpenGLStateCache::setCurrentFramebuffer(int framebuffer)
{
	m_iCurrentFramebuffer = framebuffer;
}

int OpenGLStateCache::getCurrentFramebuffer() const
{
	return m_iCurrentFramebuffer;
}

void OpenGLStateCache::setCurrentViewport(int x, int y, int width, int height)
{
	m_iViewport[0] = x;
	m_iViewport[1] = y;
	m_iViewport[2] = width;
	m_iViewport[3] = height;
}

void OpenGLStateCache::getCurrentViewport(int &x, int &y, int &width, int &height)
{
	x = m_iViewport[0];
	y = m_iViewport[1];
	width = m_iViewport[2];
	height = m_iViewport[3];
}

#endif
