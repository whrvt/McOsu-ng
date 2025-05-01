//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		OpenGLES 3.2 GLSL implementation of Shader
//
// $NoKeywords: $gles32shader
//===============================================================================//

#pragma once
#ifndef OPENGLES32SHADER_H
#define OPENGLES32SHADER_H

#include "Shader.h"

#ifdef MCENGINE_FEATURE_GLES32

class OpenGLES32Shader : public Shader
{
public:
	OpenGLES32Shader(UString shader, bool source);
	OpenGLES32Shader(UString vertexShader, UString fragmentShader, bool source); // DEPRECATED
	virtual ~OpenGLES32Shader() {destroy();}

	virtual void enable();
	virtual void disable();

	virtual void setUniform1f(UString name, float value);
	virtual void setUniform1fv(UString name, int count, float *values);
	virtual void setUniform1i(UString name, int value);
	virtual void setUniform2f(UString name, float x, float y);
	virtual void setUniform2fv(UString name, int count, float *vectors);
	virtual void setUniform3f(UString name, float x, float y, float z);
	virtual void setUniform3fv(UString name, int count, float *vectors);
	virtual void setUniform4f(UString name, float x, float y, float z, float w);
	virtual void setUniformMatrix4fv(UString name, Matrix4 &matrix);
	virtual void setUniformMatrix4fv(UString name, float *v);

	int getAttribLocation(UString name);

	// ILLEGAL:
	bool isActive();

private:
	virtual void init();
	virtual void initAsync();
	virtual void destroy();

	bool compile(UString vertexShader, UString fragmentShader, bool source);
	int createShaderFromString(UString shaderSource, int shaderType);
	int createShaderFromFile(UString fileName, int shaderType);

	UString m_sVsh, m_sFsh;

	bool m_bSource;
	int m_iVertexShader;
	int m_iFragmentShader;
	int m_iProgram;

	int m_iProgramBackup;
};

#endif

#endif
