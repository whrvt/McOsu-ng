//========== Copyright (c) 2019, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		non-switch sdl opengl es 2.x interface
//
// $NoKeywords: $SDLGLESi
//===============================================================================//

#ifndef __SWITCH__

#pragma once
#ifndef GENERICSDLGLESINTERFACE_H
#define GENERICSDLGLESINTERFACE_H

#include "cbase.h"

#if defined(MCENGINE_FEATURE_SDL) && (defined(MCENGINE_FEATURE_GLES2) || defined(MCENGINE_FEATURE_GLES32))

#include "SDLGLESInterface.h"

class GenericSDLGLESInterface : public SDLGLESInterface
{
public:
	GenericSDLGLESInterface(SDLEnvironment *environment, SDL_Window *window);
	virtual ~GenericSDLGLESInterface();
};

#endif

#endif

#endif
