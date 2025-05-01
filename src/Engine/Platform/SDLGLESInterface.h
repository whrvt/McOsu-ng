//================ Copyright (c) 2019, PG, All rights reserved. =================//
//
// Purpose:		nintendo switch opengl interface
//
// $NoKeywords: $nxgli
//===============================================================================//

#pragma once
#ifndef SDLGLESINTERFACE_H
#define SDLGLESINTERFACE_H

#include "OpenGLES2Interface.h"
#include "OpenGLES32Interface.h"

#if defined(MCENGINE_FEATURE_SDL) && (defined(MCENGINE_FEATURE_GLES2) || defined(MCENGINE_FEATURE_GLES32))
#include "SDLEnvironment.h"
#include <SDL3/SDL.h>

#ifdef MCENGINE_FEATURE_GLES2
class SDLGLESInterface : public OpenGLES2Interface
#else
class SDLGLESInterface : public OpenGLES32Interface
#endif
{
public:
	SDLGLESInterface(SDLEnvironment *environment, SDL_Window *window);
	virtual ~SDLGLESInterface();

	// scene
	virtual void endScene();

	// device settings
	virtual void setVSync(bool vsync);

private:
	SDL_Window *m_window;
	SDLEnvironment *m_env;
};

#else
#ifdef MCENGINE_FEATURE_GLES2
class SDLGLESInterface : public OpenGLES2Interface{};
#else
class SDLGLESInterface : public OpenGLES32Interface{};
#endif
#endif

#endif
