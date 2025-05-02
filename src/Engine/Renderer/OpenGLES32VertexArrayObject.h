//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		OpenGLES 3.2 baking support for vao
//
// $NoKeywords: $gles32vao
//===============================================================================//

#pragma once
#ifndef OPENGLES32VERTEXARRAYOBJECT_H
#define OPENGLES32VERTEXARRAYOBJECT_H

#include "VertexArrayObject.h"

#ifdef MCENGINE_FEATURE_GLES32

class OpenGLES32VertexArrayObject : public VertexArrayObject
{
public:
	friend class OpenGLES32Interface;
	OpenGLES32VertexArrayObject(Graphics::PRIMITIVE primitive = Graphics::PRIMITIVE::PRIMITIVE_TRIANGLES, Graphics::USAGE_TYPE usage = Graphics::USAGE_TYPE::USAGE_STATIC, bool keepInSystemMemory = false);
	~OpenGLES32VertexArrayObject() override { destroy(); }

	void draw();

	[[nodiscard]] inline unsigned int getNumTexcoords0() const { return m_iNumTexcoords; }
	[[nodiscard]] inline unsigned int getNumColors() const { return m_iNumColors; }
	[[nodiscard]] inline unsigned int getNumNormals() const { return m_iNumNormals; }

private:
	static int primitiveToOpenGL(Graphics::PRIMITIVE primitive);
	static unsigned int usageToOpenGL(Graphics::USAGE_TYPE usage);
	static forceinline Color ARGBtoABGR(Color color) { return ((color & 0xff000000) >> 0) | ((color & 0x00ff0000) >> 16) | ((color & 0x0000ff00) << 0) | ((color & 0x000000ff) << 16); }

	void init() override;
	void initAsync() override;
	void destroy() override;

	unsigned int m_iVertexBuffer;
	unsigned int m_iTexcoordBuffer;
	unsigned int m_iColorBuffer;
	unsigned int m_iNormalBuffer;

	unsigned int m_iNumTexcoords;
	unsigned int m_iNumColors;
	unsigned int m_iNumNormals;
};

#endif

#endif
