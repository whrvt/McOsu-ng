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
	virtual ~NullShader() {destroy();}

	virtual void enable() {;}
	virtual void disable() {;}

	virtual void setUniform1f(const UString &name, float value) {;}
	virtual void setUniform1fv(const UString &name, int count, float *values) {;}
	virtual void setUniform1i(const UString &name, int value) {;}
	virtual void setUniform2f(const UString &name, float x, float y) {;}
	virtual void setUniform2fv(const UString &name, int count, float *vectors) {;}
	virtual void setUniform3f(const UString &name, float x, float y, float z) {;}
	virtual void setUniform3fv(const UString &name, int count, float *vectors) {;}
	virtual void setUniform4f(const UString &name, float x, float y, float z, float w) {;}
	virtual void setUniformMatrix4fv(const UString &name, Matrix4 &matrix) {;}
	virtual void setUniformMatrix4fv(const UString &name, float *v) {;}

private:
	virtual void init() {m_bReady = true;}
	virtual void initAsync() {m_bAsyncReady = true;}
	virtual void destroy() {;}
};

#endif
