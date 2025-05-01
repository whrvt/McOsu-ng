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
SDLGLESInterface::SDLGLESInterface(SDLEnvironment *environment, SDL_Window *window) : OpenGLES2Interface()
#else
SDLGLESInterface::SDLGLESInterface(SDLEnvironment *environment, SDL_Window *window) : OpenGLES32Interface()
#endif
{
	m_window = window;
	m_env = environment;
}

SDLGLESInterface::~SDLGLESInterface()
{
}

void SDLGLESInterface::endScene()
{
#ifdef MCENGINE_FEATURE_GLES2
	OpenGLES2Interface::endSceneInternal(m_env->getSwapBehavior());
#else
	OpenGLES32Interface::endSceneInternal(m_env->getSwapBehavior());
#endif

	SDL_GL_SwapWindow(m_window);
}

void SDLGLESInterface::setVSync(bool vsync)
{
	SDL_GL_SetSwapInterval(vsync ? 1 : 0);
}

#endif
