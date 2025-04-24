//========== Copyright (c) 2019, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		non-switch sdl opengl es 2.x interface
//
// $NoKeywords: $sdlgles2i
//===============================================================================//

#ifndef __SWITCH__

#include "GenericSDLGLES2Interface.h"

#if defined(MCENGINE_FEATURE_SDL) && defined(MCENGINE_FEATURE_OPENGLES)

#include "Engine.h"

#include "OpenGLHeaders.h"

GenericSDLGLES2Interface::GenericSDLGLES2Interface(SDL_Window *window) : SDLGLES2Interface(window)
{
	// check GLEW
	//glewExperimental = GL_TRUE; // TODO: upgrade to glew >= 2.0.0 to fix this (would cause crash in e.g. glGenVertexArrays() without it)
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		debugLog("glewInit() Error: %s\n", glewGetErrorString(err));
		engine->showMessageErrorFatal("OpenGL Error", "Couldn't glewInit()!\nThe engine will exit now.");
		engine->shutdown();
		return;
	}
}

GenericSDLGLES2Interface::~GenericSDLGLES2Interface()
{
}

#endif

#endif
