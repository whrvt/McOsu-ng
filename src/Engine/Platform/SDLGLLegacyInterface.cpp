//================ Copyright (c) 2018, PG, All rights reserved. =================//
//
// Purpose:		sdl opengl interface
//
// $NoKeywords: $sdlgli
//===============================================================================//

#include "SDLGLLegacyInterface.h"

#if defined(MCENGINE_FEATURE_SDL) && defined(MCENGINE_FEATURE_OPENGL)

#include "Engine.h"
#include "ConVar.h"

ConVar gl_finish_before_swap("gl_finish_before_swap", false, FCVAR_NONE, "force GL operations to complete before swapping");

SDLGLLegacyInterface::SDLGLLegacyInterface(SDL_Window *window) : OpenGLLegacyInterface()
{
	m_window = window;
	m_glfinish = false;
	convar->getConVarByName("gl_finish_before_swap")->setCallback( fastdelegate::MakeDelegate(this, &SDLGLLegacyInterface::onSwapBehaviorChange) );
}

SDLGLLegacyInterface::~SDLGLLegacyInterface()
{
}

void SDLGLLegacyInterface::endScene()
{
	OpenGLLegacyInterface::endSceneInternal(m_glfinish);
	SDL_GL_SwapWindow(m_window);
}

void SDLGLLegacyInterface::setVSync(bool vsync)
{
	SDL_GL_SetSwapInterval(vsync ? 1 : 0);
}

#endif
