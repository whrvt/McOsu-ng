//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		empty implementation of RenderTarget
//
// $NoKeywords: $nrt
//===============================================================================//

#pragma once
#ifndef NULLRENDERTARGET_H
#define NULLRENDERTARGET_H

#include "RenderTarget.h"

class NullRenderTarget final : public RenderTarget
{
public:
	NullRenderTarget(int x, int y, int width, int height, Graphics::MULTISAMPLE_TYPE multiSampleType) : RenderTarget(x, y, width, height, multiSampleType) {;}
	~NullRenderTarget() override {destroy();}

	void enable() override {;}
	void disable() override {;}

	void bind(unsigned int textureUnit = 0) override {;}
	void unbind() override {;}

private:
	void init() override {m_bReady = true;}
	void initAsync() override {m_bAsyncReady = true;}
	void destroy() override {;}
};

#endif
