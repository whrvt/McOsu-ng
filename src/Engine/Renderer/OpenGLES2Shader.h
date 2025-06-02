//================ Copyright (c) 2019, PG, All rights reserved. =================//
//
// Purpose:		OpenGLES2 GLSL implementation of Shader
//
// $NoKeywords: $gles2shader
//===============================================================================//

#pragma once
#ifndef OPENGLES2SHADER_H
#define OPENGLES2SHADER_H

#include "Shader.h"

#ifdef MCENGINE_FEATURE_GLES2

class OpenGLES2Shader final : public Shader
{
public:
	OpenGLES2Shader(const UString &shader, bool source);
	OpenGLES2Shader(const UString &vertexShader, const UString &fragmentShader, bool source); // DEPRECATED
	virtual ~OpenGLES2Shader() {destroy();}

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

	int getAttribLocation(const UString &name);

	// ILLEGAL:
	bool isActive();

private:
	virtual void init();
	virtual void initAsync();
	virtual void destroy();

	bool compile(const UString &vertexShader, const UString &fragmentShader, bool source);
	int createShaderFromString(const UString &shaderSource, int shaderType);
	int createShaderFromFile(const UString &fileName, int shaderType);

	UString m_sVsh, m_sFsh;

	bool m_bSource;
	int m_iVertexShader;
	int m_iFragmentShader;
	int m_iProgram;

	int m_iProgramBackup;
};

#endif

#endif
