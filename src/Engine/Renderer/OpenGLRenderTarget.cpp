//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		OpenGL implementation of RenderTarget / render to texture
//
// $NoKeywords: $glrt
//===============================================================================//

#include "OpenGLRenderTarget.h"

#if defined(MCENGINE_FEATURE_OPENGL) || defined(MCENGINE_FEATURE_GLES2) || defined(MCENGINE_FEATURE_GLES32)

#include "ConVar.h"
#include "Engine.h"
#include "VertexArrayObject.h"

#include "OpenGLHeaders.h"
#include "OpenGLStateCache.h"

extern ConVar debug_opengl;

OpenGLRenderTarget::OpenGLRenderTarget(int x, int y, int width, int height, Graphics::MULTISAMPLE_TYPE multiSampleType) : RenderTarget(x, y, width, height, multiSampleType)
{
	m_iFrameBuffer = 0;
	m_iRenderTexture = 0;
	m_iDepthBuffer = 0;
	m_iResolveTexture = 0;
	m_iResolveFrameBuffer = 0;

	m_iFrameBufferBackup = 0;
	m_iTextureUnitBackup = 0;

	m_iViewportBackup[0] = 0;
	m_iViewportBackup[1] = 0;
	m_iViewportBackup[2] = 0;
	m_iViewportBackup[3] = 0;
}

void OpenGLRenderTarget::init()
{
	debugLog("Building RenderTarget (%ix%i) ...\n", (int)m_vSize.x, (int)m_vSize.y);

	m_iFrameBuffer = 0;
	m_iRenderTexture = 0;
	m_iDepthBuffer = 0;
	m_iResolveTexture = 0;
	m_iResolveFrameBuffer = 0;

	int numMultiSamples = 2;
	switch (m_multiSampleType)
	{
	case Graphics::MULTISAMPLE_TYPE::MULTISAMPLE_0X: // spec: i guess 0x isn't desirable? seems like it's handled in "if (isMultiSampled())"
		break;
	case Graphics::MULTISAMPLE_TYPE::MULTISAMPLE_2X:
		numMultiSamples = 2;
		break;
	case Graphics::MULTISAMPLE_TYPE::MULTISAMPLE_4X:
		numMultiSamples = 4;
		break;
	case Graphics::MULTISAMPLE_TYPE::MULTISAMPLE_8X:
		numMultiSamples = 8;
		break;
	case Graphics::MULTISAMPLE_TYPE::MULTISAMPLE_16X:
		numMultiSamples = 16;
		break;
	}

	// create framebuffer
	glGenFramebuffers(1, &m_iFrameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_iFrameBuffer);
	if (m_iFrameBuffer == 0)
	{
		engine->showMessageError("RenderTarget Error", "Couldn't glGenFramebuffers() or glBindFramebuffer()!");
		return;
	}

	// create depth buffer
	glGenRenderbuffers(1, &m_iDepthBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, m_iDepthBuffer);
	if (m_iDepthBuffer == 0)
	{
		engine->showMessageError("RenderTarget Error", "Couldn't glGenRenderBuffers() or glBindRenderBuffer()!");
		return;
	}

	// fill depth buffer
	constexpr auto DEPTH_COMPONENT = Env::cfg(REND::GL) ? GL_DEPTH_COMPONENT : GL_DEPTH_COMPONENT24; // GL ES needs this manually specified to avoid artifacts
#if (defined(MCENGINE_FEATURE_OPENGL) || defined(MCENGINE_FEATURE_GLES32)) && !defined(MCENGINE_PLATFORM_WASM)

	if (isMultiSampled())
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, numMultiSamples, DEPTH_COMPONENT, (int)m_vSize.x, (int)m_vSize.y);
	else
		glRenderbufferStorage(GL_RENDERBUFFER, DEPTH_COMPONENT, (int)m_vSize.x, (int)m_vSize.y);
#else

	glRenderbufferStorage(GL_RENDERBUFFER, DEPTH_COMPONENT, (int)m_vSize.x, (int)m_vSize.y);

#endif

	// set depth buffer as depth attachment on the framebuffer
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_iDepthBuffer);

	// create texture
	glGenTextures(1, &m_iRenderTexture);

#if (defined(MCENGINE_FEATURE_OPENGL) || defined(MCENGINE_FEATURE_GLES32)) && !defined(MCENGINE_PLATFORM_WASM)

	glBindTexture(isMultiSampled() ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D, m_iRenderTexture);

#else

	glBindTexture(GL_TEXTURE_2D, m_iRenderTexture);

#endif

	if (m_iRenderTexture == 0)
	{
		engine->showMessageError("RenderTarget Error", "Couldn't glGenTextures() or glBindTexture()!");
		return;
	}

	// fill texture
#if (defined(MCENGINE_FEATURE_OPENGL) || defined(MCENGINE_FEATURE_GLES32)) && !defined(MCENGINE_PLATFORM_WASM)

	if (isMultiSampled())
	{
		if constexpr (Env::cfg(REND::GL))
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, numMultiSamples, GL_RGBA8, (int)m_vSize.x, (int)m_vSize.y, true); // use fixed sample locations
		else
			glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, numMultiSamples, GL_RGBA8, (int)m_vSize.x, (int)m_vSize.y, true);
	}
	else
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (int)m_vSize.x, (int)m_vSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

		// set texture parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);             // no mipmapping atm
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // disable texture wrap
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

#else

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (int)m_vSize.x, (int)m_vSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	// set texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // disable texture wrap
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

#endif

	// set render texture as color attachment0 on the framebuffer
#if (defined(MCENGINE_FEATURE_OPENGL) || defined(MCENGINE_FEATURE_GLES32)) && !defined(MCENGINE_PLATFORM_WASM)

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, isMultiSampled() ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D, m_iRenderTexture, 0);

#else

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_iRenderTexture, 0);

#endif

	// if multisampled, create resolve framebuffer/texture
#if (defined(MCENGINE_FEATURE_OPENGL) || defined(MCENGINE_FEATURE_GLES32)) && !defined(MCENGINE_PLATFORM_WASM)

	if (isMultiSampled())
	{
		if (m_iResolveFrameBuffer == 0)
		{
			// create resolve framebuffer
			glGenFramebuffers(1, &m_iResolveFrameBuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, m_iResolveFrameBuffer);
			if (m_iResolveFrameBuffer == 0)
			{
				engine->showMessageError("RenderTarget Error", "Couldn't glGenFramebuffers() or glBindFramebuffer() multisampled!");
				return;
			}

			// create resolve texture
			glGenTextures(1, &m_iResolveTexture);
			glBindTexture(GL_TEXTURE_2D, m_iResolveTexture);
			if (m_iResolveTexture == 0)
			{
				engine->showMessageError("RenderTarget Error", "Couldn't glGenTextures() or glBindTexture() multisampled!");
				return;
			}

			// set texture parameters
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0); // no mips

			// fill texture
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (int)m_vSize.x, (int)m_vSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

			// set resolve texture as color attachment0 on the resolve framebuffer
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_iResolveTexture, 0);
		}
	}

#endif

	if (debug_opengl.getBool()) // put this behind a flag because glCheckFramebufferStatus causes unnecessary command queue syncs
	{
		// error checking
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			engine->showMessageError("RenderTarget Error", UString::format("!GL_FRAMEBUFFER_COMPLETE, size = (%ix%i), multisampled = %i, status = %u", (int)m_vSize.x, (int)m_vSize.y,
																		(int)isMultiSampled(), status));
			return;
		}
	}

	// reset bound texture and framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// check if the default framebuffer is active first before setting viewport
	if (OpenGLStateCache::getInstance().getCurrentFramebuffer() == 0)
	{
		int viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);
		OpenGLStateCache::getInstance().setCurrentViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
	}

	m_bReady = true;
}

void OpenGLRenderTarget::initAsync()
{
	m_bAsyncReady = true;
}

void OpenGLRenderTarget::destroy()
{
	if (m_iResolveTexture != 0)
		glDeleteTextures(1, &m_iResolveTexture);
	if (m_iResolveFrameBuffer != 0)
		glDeleteFramebuffers(1, &m_iResolveFrameBuffer);
	if (m_iRenderTexture != 0)
		glDeleteTextures(1, &m_iRenderTexture);
	if (m_iDepthBuffer != 0)
		glDeleteRenderbuffers(1, &m_iDepthBuffer);
	if (m_iFrameBuffer != 0)
		glDeleteFramebuffers(1, &m_iFrameBuffer);

	m_iFrameBuffer = 0;
	m_iRenderTexture = 0;
	m_iDepthBuffer = 0;
	m_iResolveTexture = 0;
	m_iResolveFrameBuffer = 0;
}

void OpenGLRenderTarget::enable()
{
	if (!m_bReady)
		return;

	// use the state cache instead of querying OpenGL directly
	m_iFrameBufferBackup = OpenGLStateCache::getInstance().getCurrentFramebuffer();
	glBindFramebuffer(GL_FRAMEBUFFER, m_iFrameBuffer);
	OpenGLStateCache::getInstance().setCurrentFramebuffer(m_iFrameBuffer);

	OpenGLStateCache::getInstance().getCurrentViewport(m_iViewportBackup[0], m_iViewportBackup[1], m_iViewportBackup[2], m_iViewportBackup[3]);

	// set new viewport
	int newViewX = -m_vPos.x;
	int newViewY = (m_vPos.y - graphics->getResolution().y) + m_vSize.y;
	int newViewWidth = graphics->getResolution().x;
	int newViewHeight = graphics->getResolution().y;

	glViewport(newViewX, newViewY, newViewWidth, newViewHeight);

	// update cache
	OpenGLStateCache::getInstance().setCurrentViewport(newViewX, newViewY, newViewWidth, newViewHeight);

	// clear
	if (debug_rt->getBool())
		glClearColor(0.0f, 0.5f, 0.0f, 0.5f);
	else
		glClearColor(Rf(m_clearColor), Gf(m_clearColor), Bf(m_clearColor), Af(m_clearColor));

	if (m_bClearColorOnDraw || m_bClearDepthOnDraw)
		glClear((m_bClearColorOnDraw ? GL_COLOR_BUFFER_BIT : 0) | (m_bClearDepthOnDraw ? GL_DEPTH_BUFFER_BIT : 0));
}

void OpenGLRenderTarget::disable()
{
	if (!m_bReady)
		return;

	// if multisampled, blit content for multisampling into resolve texture
#if (defined(MCENGINE_FEATURE_OPENGL) || defined(MCENGINE_FEATURE_GLES32)) && !defined(MCENGINE_PLATFORM_WASM)

	if (isMultiSampled())
	{
		// HACKHACK: force disable antialiasing
		graphics->setAntialiasing(false);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_iFrameBuffer);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_iResolveFrameBuffer);

		// for multisampled, the sizes MUST be the same! you can't blit from multisampled into non-multisampled or different size
		glBlitFramebuffer(0, 0, (int)m_vSize.x, (int)m_vSize.y, 0, 0, (int)m_vSize.x, (int)m_vSize.y, GL_COLOR_BUFFER_BIT, GL_LINEAR);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

		// update cache for the current framebuffer (now 0)
		OpenGLStateCache::getInstance().setCurrentFramebuffer(0);
	}

#endif

	// restore viewport
	glViewport(m_iViewportBackup[0], m_iViewportBackup[1], m_iViewportBackup[2], m_iViewportBackup[3]);

	// update the cache with restored viewport
	OpenGLStateCache::getInstance().setCurrentViewport(m_iViewportBackup[0], m_iViewportBackup[1], m_iViewportBackup[2], m_iViewportBackup[3]);

	// restore framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, m_iFrameBufferBackup);

	// update cache for the restored framebuffer
	OpenGLStateCache::getInstance().setCurrentFramebuffer(m_iFrameBufferBackup);
}

void OpenGLRenderTarget::bind(unsigned int textureUnit)
{
	if (!m_bReady)
		return;

	m_iTextureUnitBackup = textureUnit;

	// switch texture units before enabling+binding
	glActiveTexture(GL_TEXTURE0 + textureUnit);

	// set texture
	glBindTexture(GL_TEXTURE_2D, isMultiSampled() ? m_iResolveTexture : m_iRenderTexture);

	// needed for legacy support (OpenGLLegacyInterface)
	// DEPRECATED LEGACY
	if constexpr (Env::cfg(REND::GL))
		glEnable(GL_TEXTURE_2D);
}

void OpenGLRenderTarget::unbind()
{
	if (!m_bReady)
		return;

	// restore texture unit (just in case) and set to no texture
	glActiveTexture(GL_TEXTURE0 + m_iTextureUnitBackup);
	glBindTexture(GL_TEXTURE_2D, 0);

	// restore default texture unit
	if (m_iTextureUnitBackup != 0)
		glActiveTexture(GL_TEXTURE0);
}

void OpenGLRenderTarget::blitResolveFrameBufferIntoFrameBuffer(OpenGLRenderTarget *rt)
{
#if (defined(MCENGINE_FEATURE_OPENGL) || defined(MCENGINE_FEATURE_GLES32)) && !defined(MCENGINE_PLATFORM_WASM)

	if (isMultiSampled())
	{
		// HACKHACK: force disable antialiasing
		graphics->setAntialiasing(false);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_iResolveFrameBuffer);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, rt->getFrameBuffer());

		glBlitFramebuffer(0, 0, (int)m_vSize.x, (int)m_vSize.y, 0, 0, (int)rt->getWidth(), (int)rt->getHeight(), GL_COLOR_BUFFER_BIT, GL_LINEAR);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	}

#endif
}

void OpenGLRenderTarget::blitFrameBufferIntoFrameBuffer(OpenGLRenderTarget *rt)
{
#if (defined(MCENGINE_FEATURE_OPENGL) || defined(MCENGINE_FEATURE_GLES32)) && !defined(MCENGINE_PLATFORM_WASM)

	// HACKHACK: force disable antialiasing
	graphics->setAntialiasing(false);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_iFrameBuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, rt->getFrameBuffer());

	glBlitFramebuffer(0, 0, (int)m_vSize.x, (int)m_vSize.y, 0, 0, (int)rt->getWidth(), (int)rt->getHeight(), GL_COLOR_BUFFER_BIT, GL_LINEAR);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

#endif
}

#endif
