//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		OpenGL implementation of Image
//
// $NoKeywords: $glimg
//===============================================================================//

#pragma once
#ifndef OPENGLIMAGE_H
#define OPENGLIMAGE_H

#include "Image.h"

#if defined(MCENGINE_FEATURE_OPENGL) || defined (MCENGINE_FEATURE_GLES2) || defined(MCENGINE_FEATURE_GLES32) || defined(MCENGINE_FEATURE_GL3)

class OpenGLImage final : public Image
{
public:
	OpenGLImage(UString filepath, bool mipmapped = false, bool keepInSystemMemory = false);
	OpenGLImage(int width, int height, bool mipmapped = false, bool keepInSystemMemory = false);
	~OpenGLImage() override {destroy();}

	void bind(unsigned int textureUnit = 0) override;
	void unbind() override;

	void setFilterMode(Graphics::FILTER_MODE filterMode) override;
	void setWrapMode(Graphics::WRAP_MODE wrapMode) override;

private:
	void init() override;
	void initAsync() override;
	void destroy() override;

	void handleGLErrors();

	unsigned int m_GLTexture;
	unsigned int m_iTextureUnitBackup;
};

#endif

#endif
