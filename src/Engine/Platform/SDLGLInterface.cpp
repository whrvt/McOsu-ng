//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		SDL GL abstraction interface
//
// $NoKeywords: $sdlgli
//===============================================================================//

#include "SDLGLInterface.h"

#if defined(MCENGINE_FEATURE_GLES2) || defined(MCENGINE_FEATURE_GLES32) || defined(MCENGINE_FEATURE_GL3) || defined(MCENGINE_FEATURE_OPENGL)

#include <SDL3/SDL.h>

#include "Engine.h"

// shared convars
ConVar debug_opengl("debug_opengl", false, FCVAR_NONE);

SDLGLInterface::SDLGLInterface(SDL_Window *window) : BackendGLInterface()
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

SDLGLInterface::~SDLGLInterface() {}

void SDLGLInterface::endScene()
{
	BackendGLInterface::endScene();

	SDL_GL_SwapWindow(m_window);
}

void SDLGLInterface::setVSync(bool vsync)
{
	if constexpr (!Env::cfg(OS::WASM))
		SDL_GL_SetSwapInterval(vsync ? 1 : 0);
}

UString SDLGLInterface::getVendor()
{
	static const GLubyte *vendor = nullptr; 
	if (!vendor) vendor = glGetString(GL_VENDOR);
	return reinterpret_cast<const char *>(vendor);
}

UString SDLGLInterface::getModel()
{
	static const GLubyte *model = nullptr;
	if (!model) model = glGetString(GL_RENDERER);
	return reinterpret_cast<const char *>(model);
}

UString SDLGLInterface::getVersion()
{
	static const GLubyte *version = nullptr;
	if (!version) version = glGetString(GL_VERSION);
	return reinterpret_cast<const char *>(version);
}

int SDLGLInterface::getVRAMTotal()
{
	static GLint totalMem[4]{-1, -1, -1, -1};

	if (totalMem[0] == -1)
	{
		glGetIntegerv(GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, totalMem);
		if (!(totalMem[0] > 0 && glGetError() != GL_INVALID_ENUM))
			totalMem[0] = 0;
	}
	return totalMem[0];
}

int SDLGLInterface::getVRAMRemaining()
{
	GLint nvidiaMemory[4]{-1, -1, -1, -1};
	GLint atiMemory[4]{-1, -1, -1, -1};

	glGetIntegerv(GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, nvidiaMemory);

	if (nvidiaMemory[0] > 0)
		return nvidiaMemory[0];

	glGetIntegerv(TEXTURE_FREE_MEMORY_ATI, atiMemory);
	return atiMemory[0];
}

#endif
