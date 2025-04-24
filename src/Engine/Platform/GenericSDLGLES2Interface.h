//========== Copyright (c) 2019, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		non-switch sdl opengl es 2.x interface
//
// $NoKeywords: $sdlgles2i
//===============================================================================//

#ifndef __SWITCH__

#pragma once
#ifndef GENERICSDLGLES2INTERFACE_H
#define GENERICSDLGLES2INTERFACE_H

#include "cbase.h"

#if defined(MCENGINE_FEATURE_SDL) && defined(MCENGINE_FEATURE_OPENGLES)

#include "SDLGLES2Interface.h"

class GenericSDLGLES2Interface : public SDLGLES2Interface
{
public:
	GenericSDLGLES2Interface(SDL_Window *window);
	virtual ~GenericSDLGLES2Interface();
};

#endif

#endif

#endif
