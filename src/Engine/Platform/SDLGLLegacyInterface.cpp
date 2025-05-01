//================ Copyright (c) 2018, PG, All rights reserved. =================//
//
// Purpose:		sdl opengl interface
//
// $NoKeywords: $sdlgli
//===============================================================================//

#include "SDLGLLegacyInterface.h"

#if defined(MCENGINE_FEATURE_SDL) && defined(MCENGINE_FEATURE_OPENGL)

#include "Engine.h"

SDLGLLegacyInterface::SDLGLLegacyInterface(SDLEnvironment *environment, SDL_Window *window) : OpenGLLegacyInterface()
{
	m_window = window;
	m_env = environment;
}

SDLGLLegacyInterface::~SDLGLLegacyInterface()
{
}

void SDLGLLegacyInterface::endScene()
{
	OpenGLLegacyInterface::endSceneInternal(m_env->getSwapBehavior());
	SDL_GL_SwapWindow(m_window);
}

void SDLGLLegacyInterface::setVSync(bool vsync)
{
	SDL_GL_SetSwapInterval(vsync ? 1 : 0);
}

#endif
