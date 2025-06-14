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
	virtual ~NullRenderTarget() {destroy();}

	virtual void enable() {;}
	virtual void disable() {;}

	virtual void bind(unsigned int textureUnit = 0) {;}
	virtual void unbind() {;}

private:
	virtual void init() {m_bReady = true;}
	virtual void initAsync() {m_bAsyncReady = true;}
	virtual void destroy() {;}
};

#endif
