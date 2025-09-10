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
	~OpenGLShader() override {destroy();}

	void enable() override;
	void disable() override;

	void setUniform1f(const std::string_view &name, float value) override;
	void setUniform1fv(const std::string_view &name, int count, float *values) override;
	void setUniform1i(const std::string_view &name, int value) override;
	void setUniform2f(const std::string_view &name, float x, float y) override;
	void setUniform2fv(const std::string_view &name, int count, float *vectors) override;
	void setUniform3f(const std::string_view &name, float x, float y, float z) override;
	void setUniform3fv(const std::string_view &name, int count, float *vectors) override;
	void setUniform4f(const std::string_view &name, float x, float y, float z, float w) override;
	void setUniformMatrix4fv(const std::string_view &name, Matrix4 &matrix) override;
	void setUniformMatrix4fv(const std::string_view &name, float *v) override;

	// ILLEGAL:
	int getAttribLocation(const std::string_view &name);
	int getAndCacheUniformLocation(const std::string_view &name);

private:
	void init() override;
	void initAsync() override;
	void destroy() override;

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

	std::unordered_map<std::string_view, int> m_uniformLocationCache;
};

#endif

#endif
