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
	m_iInterleavedVBO = 0;
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
		// update the interleaved buffer for partial vertex/color updates
		if (m_partialUpdateVertexIndices.size() > 0 || m_partialUpdateColorIndices.size() > 0)
		{
			// for interleaved buffers, we need to update entire vertices even for partial updates
			std::vector<InterleavedVertex> updatedVertices;
			bool needsUpdate = false;

			// track which vertices need updating
			std::vector<bool> vertexNeedsUpdate(m_vertices.size(), false);

			// mark vertices for update based on vertex indices
			for (size_t i = 0; i < m_partialUpdateVertexIndices.size(); i++)
			{
				const int index = m_partialUpdateVertexIndices[i];
				if (index >= 0 && index < static_cast<int>(m_vertices.size()))
				{
					vertexNeedsUpdate[index] = true;
					needsUpdate = true;
				}
			}

			// mark vertices for update based on color indices
			for (size_t i = 0; i < m_partialUpdateColorIndices.size(); i++)
			{
				const int index = m_partialUpdateColorIndices[i];
				if (index >= 0 && index < static_cast<int>(m_colors.size()) && index < static_cast<int>(m_vertices.size()))
				{
					m_colors[index] = ARGBtoABGR(m_colors[index]);
					vertexNeedsUpdate[index] = true;
					needsUpdate = true;
				}
			}

			// only proceed if updates are needed
			if (needsUpdate)
			{
				glBindBuffer(GL_ARRAY_BUFFER, m_iInterleavedVBO);

				// update continuous ranges
				int startIndex = -1;
				for (size_t i = 0; i < vertexNeedsUpdate.size(); i++)
				{
					if (vertexNeedsUpdate[i])
					{
						if (startIndex == -1)
							startIndex = i;

						// check if this is the last vertex or the next doesn't need update
						if (i == vertexNeedsUpdate.size() - 1 || !vertexNeedsUpdate[i + 1])
						{
							int endIndex = i;
							int count = endIndex - startIndex + 1;

							// create interleaved data for this range
							std::vector<InterleavedVertex> rangeData;
							for (int j = startIndex; j <= endIndex; j++)
							{
								InterleavedVertex v;
								v.position = m_vertices[j];

								v.texCoord = Vector2(0, 0);
								if (m_texcoords.size() > 0 && m_texcoords[0].size() > j)
									v.texCoord = m_texcoords[0][j];

								v.color = 0xffffffff;
								if (m_colors.size() > j)
									v.color = m_colors[j];

								v.normal = Vector3(0, 0, 1);
								if (m_normals.size() > j)
									v.normal = m_normals[j];

								rangeData.push_back(v);
							}

							// update buffer with this range
							glBufferSubData(GL_ARRAY_BUFFER, sizeof(InterleavedVertex) * startIndex, sizeof(InterleavedVertex) * count, &(rangeData[0]));

							startIndex = -1; // reset for next range
						}
					}
				}
			}

			m_partialUpdateVertexIndices.clear();
			m_partialUpdateColorIndices.clear();
		}
	}

	if (m_iInterleavedVBO != 0 && (!m_bKeepInSystemMemory || m_bReady))
		return; // only fully load if we are not already loaded

	// full load, convert all data to interleaved format
	std::vector<InterleavedVertex> interleavedData;

	// need to convert quads to triangles
	if (m_primitive == Graphics::PRIMITIVE::PRIMITIVE_QUADS && m_vertices.size() >= 4)
	{
		for (size_t i = 0; i < m_vertices.size(); i += 4)
		{
			if (i + 3 >= m_vertices.size())
				break; // ensure we have a full quad

			// first triangle (0,1,2)
			for (int j = 0; j < 3; j++)
			{
				InterleavedVertex v;
				v.position = m_vertices[i + j];

				v.texCoord = Vector2(0, 0);
				if (m_texcoords.size() > 0 && m_texcoords[0].size() > i + j)
					v.texCoord = m_texcoords[0][i + j];

				v.color = 0xffffffff;
				if (m_colors.size() > i + j)
					v.color = ARGBtoABGR(m_colors[i + j]);

				v.normal = Vector3(0, 0, 1);
				if (m_normals.size() > i + j)
					v.normal = m_normals[i + j];

				interleavedData.push_back(v);
			}

			// second triangle (0,2,3)
			const int indices[3] = {0, 2, 3};
			for (int j = 0; j < 3; j++)
			{
				InterleavedVertex v;
				v.position = m_vertices[i + indices[j]];

				v.texCoord = Vector2(0, 0);
				if (m_texcoords.size() > 0 && m_texcoords[0].size() > i + indices[j])
					v.texCoord = m_texcoords[0][i + indices[j]];

				v.color = 0xffffffff;
				if (m_colors.size() > i + indices[j])
					v.color = ARGBtoABGR(m_colors[i + indices[j]]);

				v.normal = Vector3(0, 0, 1);
				if (m_normals.size() > i + indices[j])
					v.normal = m_normals[i + indices[j]];

				interleavedData.push_back(v);
			}
		}

		// update the primitive type to reflect the conversion
		m_primitive = Graphics::PRIMITIVE::PRIMITIVE_TRIANGLES;
	}
	else
	{
		// non-quad primitives
		for (size_t i = 0; i < m_vertices.size(); i++)
		{
			InterleavedVertex v;
			v.position = m_vertices[i];

			v.texCoord = Vector2(0, 0);
			if (m_texcoords.size() > 0 && m_texcoords[0].size() > i)
				v.texCoord = m_texcoords[0][i];

			v.color = 0xffffffff;
			if (m_colors.size() > i)
				v.color = ARGBtoABGR(m_colors[i]);

			v.normal = Vector3(0, 0, 1);
			if (m_normals.size() > i)
				v.normal = m_normals[i];

			interleavedData.push_back(v);
		}
	}

	m_iNumVertices = interleavedData.size();
	m_iNumTexcoords = (m_texcoords.size() > 0 && m_texcoords[0].size() > 0) ? m_texcoords[0].size() : 0;
	m_iNumColors = m_colors.size();
	m_iNumNormals = m_normals.size();

	// create and fill interleaved VBO
	glGenBuffers(1, &m_iInterleavedVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_iInterleavedVBO);
	glBufferData(GL_ARRAY_BUFFER, interleavedData.size() * sizeof(InterleavedVertex), interleavedData.size() > 0 ? &(interleavedData[0]) : NULL, usageToOpenGL(m_usage));

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

	if (m_iInterleavedVBO > 0)
		glDeleteBuffers(1, &m_iInterleavedVBO);

	m_iInterleavedVBO = 0;
	m_iNumTexcoords = 0;
	m_iNumColors = 0;
	m_iNumNormals = 0;
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

	glBindBuffer(GL_ARRAY_BUFFER, m_iInterleavedVBO);

	// position (always)
	glVertexAttribPointer(g->getShaderGenericAttribPosition(), 3, GL_FLOAT, GL_FALSE, sizeof(InterleavedVertex), (GLvoid *)offsetof(InterleavedVertex, position));
	glEnableVertexAttribArray(g->getShaderGenericAttribPosition());

	// texcoords
	if (m_iNumTexcoords > 0)
	{
		glVertexAttribPointer(g->getShaderGenericAttribUV(), 2, GL_FLOAT, GL_FALSE, sizeof(InterleavedVertex), (GLvoid *)offsetof(InterleavedVertex, texCoord));
		glEnableVertexAttribArray(g->getShaderGenericAttribUV());
	}
	else
	{
		glDisableVertexAttribArray(g->getShaderGenericAttribUV());
	}

	// color
	if (m_iNumColors > 0)
	{
		glVertexAttribPointer(g->getShaderGenericAttribCol(), 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(InterleavedVertex), (GLvoid *)offsetof(InterleavedVertex, color));
		glEnableVertexAttribArray(g->getShaderGenericAttribCol());
	}
	else
	{
		glDisableVertexAttribArray(g->getShaderGenericAttribCol());
	}

	// normal (if used)
	if (m_iNumNormals > 0)
	{
		glVertexAttribPointer(g->getShaderGenericAttribNormal(), 3, GL_FLOAT, GL_FALSE, sizeof(InterleavedVertex), (GLvoid *)offsetof(InterleavedVertex, normal));
		glEnableVertexAttribArray(g->getShaderGenericAttribNormal());
	}
	else
	{
		glDisableVertexAttribArray(g->getShaderGenericAttribNormal());
	}

	// draw the geometry
	glDrawArrays(primitiveToOpenGL(m_primitive), start, end - start);

	// restore default state
	glBindBuffer(GL_ARRAY_BUFFER, g->getInterleavedVBO());

	// reset attribs
	glVertexAttribPointer(g->getShaderGenericAttribPosition(), 3, GL_FLOAT, GL_FALSE, sizeof(InterleavedVertex), (GLvoid *)offsetof(InterleavedVertex, position));
	glVertexAttribPointer(g->getShaderGenericAttribUV(), 2, GL_FLOAT, GL_FALSE, sizeof(InterleavedVertex), (GLvoid *)offsetof(InterleavedVertex, texCoord));
	glVertexAttribPointer(g->getShaderGenericAttribCol(), 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(InterleavedVertex), (GLvoid *)offsetof(InterleavedVertex, color));
	glVertexAttribPointer(g->getShaderGenericAttribNormal(), 3, GL_FLOAT, GL_FALSE, sizeof(InterleavedVertex), (GLvoid *)offsetof(InterleavedVertex, normal));

	glEnableVertexAttribArray(g->getShaderGenericAttribPosition());
	glEnableVertexAttribArray(g->getShaderGenericAttribUV());
	glEnableVertexAttribArray(g->getShaderGenericAttribCol());
	glEnableVertexAttribArray(g->getShaderGenericAttribNormal());
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
