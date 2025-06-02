//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		OpenGL GLSL implementation of Shader
//
// $NoKeywords: $glshader
//===============================================================================//

#pragma once
#ifndef OPENGLSHADER_H
#define OPENGLSHADER_H

#include "Shader.h"

#ifdef MCENGINE_FEATURE_OPENGL

class OpenGLShader final : public Shader
{
public:
	OpenGLShader(const UString &shader, bool source);
	OpenGLShader(const UString &vertexShader, const UString &fragmentShader, bool source); // DEPRECATED
	virtual ~OpenGLShader() {destroy();}

	virtual void enable();
	virtual void disable();

	virtual void setUniform1f(const UString &name, float value);
	virtual void setUniform1fv(const UString &name, int count, float *values);
	virtual void setUniform1i(const UString &name, int value);
	virtual void setUniform2f(const UString &name, float x, float y);
	virtual void setUniform2fv(const UString &name, int count, float *vectors);
	virtual void setUniform3f(const UString &name, float x, float y, float z);
	virtual void setUniform3fv(const UString &name, int count, float *vectors);
	virtual void setUniform4f(const UString &name, float x, float y, float z, float w);
	virtual void setUniformMatrix4fv(const UString &name, Matrix4 &matrix);
	virtual void setUniformMatrix4fv(const UString &name, float *v);

	// ILLEGAL:
	int getAttribLocation(const UString &name);
	int getAndCacheUniformLocation(const UString &name);

private:
	virtual void init();
	virtual void initAsync();
	virtual void destroy();

private:
	bool compile(const UString &vertexShader, const UString &fragmentShader, bool source);
	int createShaderFromString(const UString &shaderSource, int shaderType);
	int createShaderFromFile(const UString &fileName, int shaderType);

private:
	bool m_bIsShader2;

	UString m_sVsh;
	UString m_sFsh;

	bool m_bSource;
	int m_iVertexShader;
	int m_iFragmentShader;
	int m_iProgram;

	int m_iProgramBackup;

	std::unordered_map<std::string, int> m_uniformLocationCache;
	std::string m_sTempStringBuffer;
};

#endif

#endif
