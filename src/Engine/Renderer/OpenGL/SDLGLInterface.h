//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		SDL GL abstraction interface
//
// $NoKeywords: $sdlgli
//===============================================================================//

#pragma once
#ifndef SDLGLINTERFACE_H
#define SDLGLINTERFACE_H

#include "EngineFeatures.h"

#if defined(MCENGINE_FEATURE_GLES32) || defined(MCENGINE_FEATURE_GL3) || defined(MCENGINE_FEATURE_OPENGL)

#include "OpenGLHeaders.h"

typedef struct SDL_Window SDL_Window;
class SDLGLInterface final : public BackendGLInterface
{
	friend class Environment;
public:
	SDLGLInterface(SDL_Window *window) : BackendGLInterface(), m_window(window) {}
	~SDLGLInterface() override {;}

	// scene
	void endScene() override;

	// device settings
	void setVSync(bool vsync) override;

	// device info
	UString getVendor() override;
	UString getModel() override;
	UString getVersion() override;
	int getVRAMRemaining() override;
	int getVRAMTotal() override;

private:
	static void load();
	SDL_Window *m_window;
};

#else
class SDLGLInterface
{};
#endif

#endif
