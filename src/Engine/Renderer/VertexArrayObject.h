//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		modern opengl style mesh wrapper (vertices, texcoords, etc.)
//
// $NoKeywords: $vao
//===============================================================================//

#pragma once
#ifndef VERTEXARRAYOBJECT_H
#define VERTEXARRAYOBJECT_H

#include "Resource.h"

class VertexArrayObject : public Resource
{
public:
	VertexArrayObject(Graphics::PRIMITIVE primitive = Graphics::PRIMITIVE::PRIMITIVE_TRIANGLES, Graphics::USAGE_TYPE usage = Graphics::USAGE_TYPE::USAGE_STATIC, bool keepInSystemMemory = false);

	inline void clear() { empty(); }
	void empty();

	void addVertex(Vector2 v);
	void addVertex(Vector3 v);
	void addVertex(float x, float y, float z = 0);

	void addTexcoord(Vector2 uv, unsigned int textureUnit = 0);
	void addTexcoord(float u, float v, unsigned int textureUnit = 0);

	void addNormal(Vector3 normal);
	void addNormal(float x, float y, float z);

	void addColor(Color color);

	void setVertex(int index, Vector2 v);
	void setVertex(int index, Vector3 v);
	void setVertex(int index, float x, float y, float z = 0);
	void setVertices(const std::vector<Vector3> &vertices);
	void setTexcoords(const std::vector<Vector2> &texcoords, unsigned int textureUnit = 0);
	inline void setNormals(const std::vector<Vector3> &normals) { m_normals = normals; }
	inline void setColors(const std::vector<Color> &colors) { m_colors = colors; }

	void setColor(int index, Color color);

	void setType(Graphics::PRIMITIVE primitive);
	void setDrawRange(int fromIndex, int toIndex);
	void setDrawPercent(float fromPercent = 0.0f, float toPercent = 1.0f, int nearestMultiple = 0); // DEPRECATED

	[[nodiscard]] inline Graphics::PRIMITIVE getPrimitive() const {return m_primitive;}
	[[nodiscard]] inline Graphics::USAGE_TYPE getUsage() const {return m_usage;}

	[[nodiscard]] inline const std::vector<Vector3> &getVertices() const {return m_vertices;}
	[[nodiscard]] inline const std::vector<std::vector<Vector2>> &getTexcoords() const {return m_texcoords;}
	[[nodiscard]] inline const std::vector<Vector3> &getNormals() const {return m_normals;}
	[[nodiscard]] inline const std::vector<Color> &getColors() const {return m_colors;}

	[[nodiscard]] inline unsigned int getNumVertices() const {return m_iNumVertices;}
	[[nodiscard]] inline bool hasTexcoords() const {return m_bHasTexcoords;}

	virtual void draw() { assert(false); } // implementation dependent (gl/dx11/etc.)

	// type inspection
	[[nodiscard]] Type getResType() const final { return VAO; }

	VertexArrayObject *asVAO() final { return this; }
	[[nodiscard]] const VertexArrayObject *asVAO() const final { return this; }

protected:
	static int nearestMultipleUp(int number, int multiple);
	static int nearestMultipleDown(int number, int multiple);

	void init() override;
	void initAsync() override;
	void destroy() override;

	void updateTexcoordArraySize(unsigned int textureUnit);

	Graphics::PRIMITIVE m_primitive;
	Graphics::USAGE_TYPE m_usage;
	bool m_bKeepInSystemMemory;

	std::vector<Vector3> m_vertices;
	std::vector<std::vector<Vector2>> m_texcoords;
	std::vector<Vector3> m_normals;
	std::vector<Color> m_colors;

	unsigned int m_iNumVertices;
	bool m_bHasTexcoords;

	std::vector<int> m_partialUpdateVertexIndices;
	std::vector<int> m_partialUpdateColorIndices;

	// custom
	int m_iDrawRangeFromIndex;
	int m_iDrawRangeToIndex;
	int m_iDrawPercentNearestMultiple;
	float m_fDrawPercentFromPercent;
	float m_fDrawPercentToPercent;
};

#endif
