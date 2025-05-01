//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		OpenGLES 3.2 baking support for vao
//
// $NoKeywords: $gles32vao
//===============================================================================//

#include "OpenGLES32VertexArrayObject.h"

#ifdef MCENGINE_FEATURE_GLES32

#include "Engine.h"
#include "OpenGLES32Interface.h"

#include "OpenGLHeaders.h"

OpenGLES32VertexArrayObject::OpenGLES32VertexArrayObject(Graphics::PRIMITIVE primitive, Graphics::USAGE_TYPE usage, bool keepInSystemMemory) : VertexArrayObject(primitive, usage, keepInSystemMemory)
{
	m_iVertexBuffer = 0;
	m_iTexcoordBuffer = 0;
	m_iColorBuffer = 0;
	m_iNormalBuffer = 0;

	m_iNumTexcoords = 0;
	m_iNumColors = 0;
	m_iNumNormals = 0;
}

void OpenGLES32VertexArrayObject::init()
{
	if (!(m_bAsyncReady.load()) || m_vertices.size() < 2)
		return;

	// handle partial reloads
	if (m_bReady)
	{
		// update vertex buffer
		if (m_partialUpdateVertexIndices.size() > 0)
		{
			glBindBuffer(GL_ARRAY_BUFFER, m_iVertexBuffer);
			for (size_t i = 0; i < m_partialUpdateVertexIndices.size(); i++)
			{
				const int offsetIndex = m_partialUpdateVertexIndices[i];

				// group by continuous chunks to reduce calls
				int numContinuousIndices = 1;
				while ((i + 1) < m_partialUpdateVertexIndices.size())
				{
					if ((m_partialUpdateVertexIndices[i + 1] - m_partialUpdateVertexIndices[i]) == 1)
					{
						numContinuousIndices++;
						i++;
					}
					else
						break;
				}

				glBufferSubData(GL_ARRAY_BUFFER, sizeof(Vector3) * offsetIndex, sizeof(Vector3) * numContinuousIndices, &(m_vertices[offsetIndex]));
			}
			m_partialUpdateVertexIndices.clear();
		}

		// update color buffer
		if (m_partialUpdateColorIndices.size() > 0)
		{
			glBindBuffer(GL_ARRAY_BUFFER, m_iColorBuffer);
			for (size_t i = 0; i < m_partialUpdateColorIndices.size(); i++)
			{
				const int offsetIndex = m_partialUpdateColorIndices[i];

				m_colors[offsetIndex] = ARGBtoABGR(m_colors[offsetIndex]);

				// group by continuous chunks to reduce calls
				int numContinuousIndices = 1;
				while ((i + 1) < m_partialUpdateColorIndices.size())
				{
					if ((m_partialUpdateColorIndices[i + 1] - m_partialUpdateColorIndices[i]) == 1)
					{
						numContinuousIndices++;
						i++;

						m_colors[m_partialUpdateColorIndices[i]] = ARGBtoABGR(m_colors[m_partialUpdateColorIndices[i]]);
					}
					else
						break;
				}

				glBufferSubData(GL_ARRAY_BUFFER, sizeof(Color) * offsetIndex, sizeof(Color) * numContinuousIndices, &(m_colors[offsetIndex]));
			}
			m_partialUpdateColorIndices.clear();
		}
	}

	if (m_iVertexBuffer != 0 && (!m_bKeepInSystemMemory || m_bReady))
		return; // only fully load if we are not already loaded

	// handle full loads

	// build and fill vertex buffer
	glGenBuffers(1, &m_iVertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_iVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3) * m_vertices.size(), &(m_vertices[0]), usageToOpenGL(m_usage));

	// build and fill texcoord buffer
	if (m_texcoords.size() > 0 && m_texcoords[0].size() > 0)
	{
		m_iNumTexcoords = m_texcoords[0].size();

		glGenBuffers(1, &m_iTexcoordBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, m_iTexcoordBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vector2) * m_texcoords[0].size(), &(m_texcoords[0][0]), usageToOpenGL(m_usage));
	}

	// build and fill color buffer
	if (m_colors.size() > 0)
	{
		m_iNumColors = m_colors.size();

		// convert ARGB to ABGR for OpenGL
		for (size_t i = 0; i < m_colors.size(); i++)
		{
			m_colors[i] = ARGBtoABGR(m_colors[i]);
		}

		glGenBuffers(1, &m_iColorBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, m_iColorBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Color) * m_colors.size(), &(m_colors[0]), usageToOpenGL(m_usage));
	}

	// build and fill normal buffer
	if (m_normals.size() > 0)
	{
		m_iNumNormals = m_normals.size();

		glGenBuffers(1, &m_iNormalBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, m_iNormalBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3) * m_normals.size(), &(m_normals[0]), usageToOpenGL(m_usage));
	}

	// free memory
	if (!m_bKeepInSystemMemory)
		clear();

	m_bReady = true;
}

void OpenGLES32VertexArrayObject::initAsync()
{
	m_bAsyncReady = true;
}

void OpenGLES32VertexArrayObject::destroy()
{
	VertexArrayObject::destroy();

	if (m_iVertexBuffer > 0)
		glDeleteBuffers(1, &m_iVertexBuffer);

	if (m_iTexcoordBuffer > 0)
		glDeleteBuffers(1, &m_iTexcoordBuffer);

	if (m_iColorBuffer > 0)
		glDeleteBuffers(1, &m_iColorBuffer);

	if (m_iNormalBuffer > 0)
		glDeleteBuffers(1, &m_iNormalBuffer);

	m_iVertexBuffer = 0;
	m_iTexcoordBuffer = 0;
	m_iColorBuffer = 0;
	m_iNormalBuffer = 0;
}

void OpenGLES32VertexArrayObject::draw()
{
	if (!m_bReady)
	{
		debugLog("WARNING: called, but was not ready!\n");
		return;
	}

	const int start = clamp<int>(m_iDrawRangeFromIndex > -1 ? m_iDrawRangeFromIndex : nearestMultipleUp((int)(m_iNumVertices * m_fDrawPercentFromPercent), m_iDrawPercentNearestMultiple), 0, m_iNumVertices);
	const int end = clamp<int>(m_iDrawRangeToIndex > -1 ? m_iDrawRangeToIndex : nearestMultipleDown((int)(m_iNumVertices * m_fDrawPercentToPercent), m_iDrawPercentNearestMultiple), 0, m_iNumVertices);

	if (start > end || std::abs(end - start) == 0)
		return;

	OpenGLES32Interface *g = (OpenGLES32Interface *)engine->getGraphics();

	// configure shader state for our vertex attributes
	if (m_iNumColors > 0)
	{
		glEnableVertexAttribArray(g->getShaderGenericAttribCol());
	}
	else
	{
		glDisableVertexAttribArray(g->getShaderGenericAttribCol());
	}

	// set vertex attribute pointers
	glBindBuffer(GL_ARRAY_BUFFER, m_iVertexBuffer);
	glVertexAttribPointer(g->getShaderGenericAttribPosition(), 3, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);

	if (m_iNumTexcoords > 0)
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_iTexcoordBuffer);
		glVertexAttribPointer(g->getShaderGenericAttribUV(), 2, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);
	}

	if (m_iNumColors > 0)
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_iColorBuffer);
		glVertexAttribPointer(g->getShaderGenericAttribCol(), 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (GLvoid *)0);
	}

	// draw the geometry
	glDrawArrays(primitiveToOpenGL(m_primitive), start, end - start);

	// restore default state
	glBindBuffer(GL_ARRAY_BUFFER, g->getVBOVertices());
	glVertexAttribPointer(g->getShaderGenericAttribPosition(), 3, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);

	glBindBuffer(GL_ARRAY_BUFFER, g->getVBOTexcoords());
	glVertexAttribPointer(g->getShaderGenericAttribUV(), 2, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);

	// always enable color attrib as the default state
	glBindBuffer(GL_ARRAY_BUFFER, g->getVBOTexcolors());
	glVertexAttribPointer(g->getShaderGenericAttribCol(), 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (GLvoid *)0);
	glEnableVertexAttribArray(g->getShaderGenericAttribCol());
}

int OpenGLES32VertexArrayObject::primitiveToOpenGL(Graphics::PRIMITIVE primitive)
{
	switch (primitive)
	{
	case Graphics::PRIMITIVE::PRIMITIVE_LINES:
		return GL_LINES;
	case Graphics::PRIMITIVE::PRIMITIVE_LINE_STRIP:
		return GL_LINE_STRIP;
	case Graphics::PRIMITIVE::PRIMITIVE_TRIANGLES:
		return GL_TRIANGLES;
	case Graphics::PRIMITIVE::PRIMITIVE_TRIANGLE_FAN:
		return GL_TRIANGLE_FAN;
	case Graphics::PRIMITIVE::PRIMITIVE_TRIANGLE_STRIP:
		return GL_TRIANGLE_STRIP;
	case Graphics::PRIMITIVE::PRIMITIVE_QUADS:
		return 0; // not supported
	}

	return GL_TRIANGLES;
}

unsigned int OpenGLES32VertexArrayObject::usageToOpenGL(Graphics::USAGE_TYPE usage)
{
	switch (usage)
	{
	case Graphics::USAGE_TYPE::USAGE_STATIC:
		return GL_STATIC_DRAW;
	case Graphics::USAGE_TYPE::USAGE_DYNAMIC:
		return GL_DYNAMIC_DRAW;
	case Graphics::USAGE_TYPE::USAGE_STREAM:
		return GL_STREAM_DRAW;
	}

	return GL_STATIC_DRAW;
}

Color OpenGLES32VertexArrayObject::ARGBtoABGR(Color color)
{
	return ((color & 0xff000000) >> 0) | ((color & 0x00ff0000) >> 16) | ((color & 0x0000ff00) << 0) | ((color & 0x000000ff) << 16);
}

#endif
