//========== Copyright (c) 2019, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		non-switch sdl opengl es interface
//
// $NoKeywords: $sdlglesi
//===============================================================================//

#ifndef __SWITCH__

#include "GenericSDLGLESInterface.h"

#if defined(MCENGINE_FEATURE_SDL) && (defined(MCENGINE_FEATURE_GLES2) || defined(MCENGINE_FEATURE_GLES32))

#include "Engine.h"

#include "OpenGLHeaders.h"

GenericSDLGLESInterface::GenericSDLGLESInterface(SDL_Window *window) : SDLGLESInterface(window)
{
	// check GLEW
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		debugLog("glewInit() Error: %s\n", glewGetErrorString(err));
		engine->showMessageErrorFatal("OpenGL Error", "Couldn't glewInit()!\nThe engine will exit now.");
		engine->shutdown();
		return;
	}
}

GenericSDLGLESInterface::~GenericSDLGLESInterface()
{
}

#endif

#endif
