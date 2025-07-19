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
	friend class OpenGLLegacyInterface;
	friend class OpenGLVertexArrayObject;
	friend class OpenGLShader;
	friend class OpenGLES32Interface;
	friend class OpenGLES32VertexArrayObject;
	friend class OpenGLES32Shader;
	friend class OpenGL3Interface;
	friend class OpenGL3VertexArrayObject;
	friend class OpenGLShader;
public:
	SDLGLInterface(SDL_Window *window) : BackendGLInterface(), m_window(window) {}

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

protected:
	static std::unordered_map<Graphics::PRIMITIVE, int> primitiveToOpenGLMap;
	static std::unordered_map<Graphics::COMPARE_FUNC, int> compareFuncToOpenGLMap;
	static std::unordered_map<Graphics::USAGE_TYPE, unsigned int> usageToOpenGLMap;
private:
	static void load();
	SDL_Window *m_window;
};

#else
class SDLGLInterface
{};
#endif

#endif
