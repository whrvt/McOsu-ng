//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		empty implementation of Shader
//
// $NoKeywords: $nshader
//===============================================================================//

#pragma once
#ifndef NULLSHADER_H
#define NULLSHADER_H

#include "Shader.h"

class NullShader final : public Shader
{
public:
	NullShader(const UString &shader, bool source) {;}
	NullShader(const UString &vertexShader, const UString &fragmentShader, bool source) : Shader() {;} // DEPRECATED
	~NullShader() override {destroy();}

	void enable() override {;}
	void disable() override {;}

	void setUniform1f(const UString &name, float value) override {;}
	void setUniform1fv(const UString &name, int count, float *values) override {;}
	void setUniform1i(const UString &name, int value) override {;}
	void setUniform2f(const UString &name, float x, float y) override {;}
	void setUniform2fv(const UString &name, int count, float *vectors) override {;}
	void setUniform3f(const UString &name, float x, float y, float z) override {;}
	void setUniform3fv(const UString &name, int count, float *vectors) override {;}
	void setUniform4f(const UString &name, float x, float y, float z, float w) override {;}
	void setUniformMatrix4fv(const UString &name, Matrix4 &matrix) override {;}
	void setUniformMatrix4fv(const UString &name, float *v) override {;}

private:
	void init() override {m_bReady = true;}
	void initAsync() override {m_bAsyncReady = true;}
	void destroy() override {;}
};

#endif
