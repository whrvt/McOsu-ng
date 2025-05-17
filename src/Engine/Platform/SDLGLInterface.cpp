//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		SDL GL abstraction interface
//
// $NoKeywords: $sdlgli
//===============================================================================//

#include "SDLGLInterface.h"
#include <SDL3/SDL.h>

#if defined(MCENGINE_FEATURE_GLES2) || defined(MCENGINE_FEATURE_GLES32) || defined(MCENGINE_FEATURE_OPENGL)

#include "OpenGLHeaders.h"
#include "OpenGLStateCache.h"
#include "Engine.h"


#ifdef MCENGINE_FEATURE_GLES2
SDLGLInterface::SDLGLInterface(SDL_Window *window) : OpenGLES2Interface()
#elif defined(MCENGINE_FEATURE_GLES32)
SDLGLInterface::SDLGLInterface(SDL_Window *window) : OpenGLES32Interface()
#else
SDLGLInterface::SDLGLInterface(SDL_Window *window) : OpenGLLegacyInterface()
#endif
{
	// resolve GL functions
	glewExperimental = true;
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		debugLog("glewInit() Error: %s\n", glewGetErrorString(err));
		engine->showMessageErrorFatal("OpenGL Error", "Couldn't glewInit()!\nThe engine will exit now.");
		engine->shutdown();
	}
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
	if constexpr (Env::cfg(OS::WASM))
		SDL_GL_SetSwapInterval(1);
	else
		SDL_GL_SetSwapInterval(vsync ? 1 : 0);
}

#endif
