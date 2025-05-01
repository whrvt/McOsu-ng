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
	virtual ~OpenGLES32VertexArrayObject() { destroy(); }

	void draw();

	inline unsigned int const getNumTexcoords0() const { return m_iNumTexcoords; }
	inline unsigned int const getNumColors() const { return m_iNumColors; }
	inline unsigned int const getNumNormals() const { return m_iNumNormals; }

private:
	virtual void init();
	virtual void initAsync();
	virtual void destroy();

	static int primitiveToOpenGL(Graphics::PRIMITIVE primitive);
	static unsigned int usageToOpenGL(Graphics::USAGE_TYPE usage);
	static Color ARGBtoABGR(Color color);

	unsigned int m_iInterleavedVBO;

	// counts for attribute availability
	unsigned int m_iNumTexcoords;
	unsigned int m_iNumColors;
	unsigned int m_iNumNormals;
};

#endif

#endif
