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

class OpenGLES32Shader final : public Shader
{
public:
	OpenGLES32Shader(const UString &shader, bool source);
	OpenGLES32Shader(const UString &vertexShader, const UString &fragmentShader, bool source); // DEPRECATED
	virtual ~OpenGLES32Shader() {destroy();}

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
	int getAndCacheUniformLocation(const UString &name);

	UString m_sVsh, m_sFsh;

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
