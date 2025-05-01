//================ Copyright (c) 2019, PG, All rights reserved. =================//
//
// Purpose:		nintendo switch opengl interface
//
// $NoKeywords: $nxgli
//===============================================================================//

#include "SDLGLESInterface.h"

#if defined(MCENGINE_FEATURE_SDL) && (defined(MCENGINE_FEATURE_GLES2) || defined(MCENGINE_FEATURE_GLES32))

#include "SDLEnvironment.h"

#ifdef MCENGINE_FEATURE_GLES2
SDLGLESInterface::SDLGLESInterface(SDL_Window *window) : OpenGLES2Interface()
#else
SDLGLESInterface::SDLGLESInterface(SDL_Window *window) : OpenGLES32Interface()
#endif
{
	m_window = window;
}

SDLGLESInterface::~SDLGLESInterface()
{
}

void SDLGLESInterface::endScene()
{
#ifdef MCENGINE_FEATURE_GLES2
	OpenGLES2Interface::endScene();
#else
	OpenGLES32Interface::endScene();
#endif

	SDL_GL_SwapWindow(m_window);
}

void SDLGLESInterface::setVSync(bool vsync)
{
	SDL_GL_SetSwapInterval(vsync ? 1 : 0);
}

#endif
