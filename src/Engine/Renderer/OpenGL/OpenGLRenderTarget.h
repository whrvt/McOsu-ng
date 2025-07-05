//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		OpenGL implementation of RenderTarget / render to texture
//
// $NoKeywords: $glrt
//===============================================================================//

#pragma once
#ifndef OPENGLRENDERTARGET_H
#define OPENGLRENDERTARGET_H

#include "RenderTarget.h"

#if defined(MCENGINE_FEATURE_OPENGL) || defined (MCENGINE_FEATURE_GLES2) || defined(MCENGINE_FEATURE_GLES32) || defined(MCENGINE_FEATURE_GL3)

class OpenGLRenderTarget final : public RenderTarget
{
public:
	OpenGLRenderTarget(int x, int y, int width, int height, Graphics::MULTISAMPLE_TYPE multiSampleType = Graphics::MULTISAMPLE_TYPE::MULTISAMPLE_0X);
	~OpenGLRenderTarget() override {destroy();}

	void enable() override;
	void disable() override;

	void bind(unsigned int textureUnit = 0) override;
	void unbind() override;

	// ILLEGAL:
	void blitResolveFrameBufferIntoFrameBuffer(OpenGLRenderTarget *rt);
	void blitFrameBufferIntoFrameBuffer(OpenGLRenderTarget *rt);
	[[nodiscard]] inline unsigned int getFrameBuffer() const {return m_iFrameBuffer;}
	[[nodiscard]] inline unsigned int getRenderTexture() const {return m_iRenderTexture;}
	[[nodiscard]] inline unsigned int getResolveFrameBuffer() const {return m_iResolveFrameBuffer;}
	[[nodiscard]] inline unsigned int getResolveTexture() const {return m_iResolveTexture;}

private:
	void init() override;
	void initAsync() override;
	void destroy() override;

	unsigned int m_iFrameBuffer;
	unsigned int m_iRenderTexture;
	unsigned int m_iDepthBuffer;
	unsigned int m_iResolveFrameBuffer;
	unsigned int m_iResolveTexture;

	int m_iFrameBufferBackup;
	unsigned int m_iTextureUnitBackup;
	int m_iViewportBackup[4];
};

#endif

#endif
