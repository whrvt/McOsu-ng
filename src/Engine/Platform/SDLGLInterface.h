//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		SDL GL abstraction interface
//
// $NoKeywords: $sdlgli
//===============================================================================//

#pragma once
#ifndef SDLGLINTERFACE_H
#define SDLGLINTERFACE_H

#include "OpenGLES2Interface.h"
#include "OpenGLES32Interface.h"
#include "OpenGLLegacyInterface.h"

#if defined(MCENGINE_FEATURE_GLES2) || defined(MCENGINE_FEATURE_GLES32) || defined(MCENGINE_FEATURE_OPENGL)
typedef struct SDL_Window SDL_Window;

#ifdef MCENGINE_FEATURE_GLES2
class SDLGLInterface : public OpenGLES2Interface
#elif defined(MCENGINE_FEATURE_GLES32)
class SDLGLInterface : public OpenGLES32Interface
#else
class SDLGLInterface : public OpenGLLegacyInterface
#endif
{
public:
	SDLGLInterface(SDL_Window *window);
	~SDLGLInterface() override;

	// scene
	void endScene() override;

	// device settings
	void setVSync(bool vsync) override;

private:
	SDL_Window *m_window;
};

#else
#ifdef MCENGINE_FEATURE_GLES2
class SDLGLInterface : public OpenGLES2Interface{};
#else
class SDLGLInterface : public OpenGLES32Interface{};
#endif
#endif

#endif
