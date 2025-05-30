//================ Copyright (c) 2012, PG, All rights reserved. =================//
//
// Purpose:		shader wrapper
//
// $NoKeywords: $shader
//===============================================================================//

#pragma once
#ifndef SHADER_H
#define SHADER_H

#include "Resource.h"

class ConVar;

class Shader : public Resource
{
public:
	

public:
	Shader() : Resource() {;}
	~Shader() override {;}

	virtual void enable() = 0;
	virtual void disable() = 0;

	virtual void setUniform1f(UString name, float value) = 0;
	virtual void setUniform1fv(UString name, int count, float *values) = 0;
	virtual void setUniform1i(UString name, int value) = 0;
	virtual void setUniform2f(UString name, float x, float y) = 0;
	virtual void setUniform2fv(UString name, int count, float *vectors) = 0;
	virtual void setUniform3f(UString name, float x, float y, float z) = 0;
	virtual void setUniform3fv(UString name, int count, float *vectors) = 0;
	virtual void setUniform4f(UString name, float x, float y, float z, float w) = 0;
	virtual void setUniformMatrix4fv(UString name, Matrix4 &matrix) = 0;
	virtual void setUniformMatrix4fv(UString name, float *v) = 0;

	// type inspection
	[[nodiscard]] Type getResType() const final { return SHADER; }

	Shader *asShader() final { return this; }
	[[nodiscard]] const Shader *asShader() const final { return this; }
protected:
	void init() override = 0;
	void initAsync() override = 0;
	void destroy() override = 0;

protected:
	struct SHADER_PARSE_RESULT
	{
		UString source;
		std::vector<UString> descs;
	};

	SHADER_PARSE_RESULT parseShaderFromFileOrString(UString graphicsInterfaceAndShaderTypePrefix, UString shaderSourceOrFilePath, bool source);
};

#endif
