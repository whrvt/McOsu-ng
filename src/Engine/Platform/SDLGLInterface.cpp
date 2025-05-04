//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		SDL GL abstraction interface
//
// $NoKeywords: $sdlgli
//===============================================================================//

#include "SDLGLInterface.h"
#include <SDL3/SDL.h>

#if defined(MCENGINE_FEATURE_GLES2) || defined(MCENGINE_FEATURE_GLES32) || defined(MCENGINE_FEATURE_OPENGL)
#ifndef __SWITCH__
#include "OpenGLHeaders.h"
#include "Engine.h"
#endif

#ifdef MCENGINE_FEATURE_GLES2
SDLGLInterface::SDLGLInterface(SDL_Window *window) : OpenGLES2Interface()
#elif defined(MCENGINE_FEATURE_GLES32)
SDLGLInterface::SDLGLInterface(SDL_Window *window) : OpenGLES32Interface()
#else
SDLGLInterface::SDLGLInterface(SDL_Window *window) : OpenGLLegacyInterface()
#endif
{
#ifndef __SWITCH__
	// resolve GL functions
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		debugLog("glewInit() Error: %s\n", glewGetErrorString(err));
		engine->showMessageErrorFatal("OpenGL Error", "Couldn't glewInit()!\nThe engine will exit now.");
		engine->shutdown();
	}	
#endif
	m_window = window;
}

SDLGLInterface::~SDLGLInterface()
{
}

void SDLGLInterface::endScene()
{
#ifdef MCENGINE_FEATURE_GLES2
	OpenGLES2Interface::endScene();
#elif defined(MCENGINE_FEATURE_GLES32)
	OpenGLES32Interface::endScene();
#else
	OpenGLLegacyInterface::endScene();
#endif

	SDL_GL_SwapWindow(m_window);
}

void SDLGLInterface::setVSync(bool vsync)
{
	SDL_GL_SetSwapInterval(vsync ? 1 : 0);
}

#endif
