//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		a collection of all necessary OpenGL header files (and related)
//
// $NoKeywords: $glh
//===============================================================================//

#pragma once
#ifndef OPENGLHEADERS_H
#define OPENGLHEADERS_H

// required on windows due to missing APIENTRY typedefs
#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__CYGWIN__) || defined(__CYGWIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__)

#include <windows.h>

#define GLEW_STATIC
#include <GL/glew.h>

#define WGL_WGLEXT_PROTOTYPES // only used for wglDX*() interop stuff atm
#include <GL/wglew.h>

#include <GL/glu.h>
#include <GL/gl.h>

#include <GL/wglext.h>
#include <GL/glext.h>

#endif

#ifdef __EMSCRIPTEN__
#include <GL/glew.h>
#endif

#ifdef __linux__

#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>

#endif

#ifdef __APPLE__

#define GLEW_STATIC
#include <glew.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>

#endif

#define GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX			0x9047
#define GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX		0x9048
#define GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX	0x9049

#define VBO_FREE_MEMORY_ATI								0x87FB
#define TEXTURE_FREE_MEMORY_ATI							0x87FC
#define RENDERBUFFER_FREE_MEMORY_ATI					0x87FD

#ifdef MCENGINE_FEATURE_OPENGL
	#include "OpenGLLegacyInterface.h"
	#include "OpenGLVertexArrayObject.h"
	#include "OpenGLShader.h"

	using BackendGLInterface = OpenGLLegacyInterface;
	using BackendGLVAO = OpenGLVertexArrayObject;
	using BackendGLShader = OpenGLShader;
#elif defined(MCENGINE_FEATURE_GLES2)
	#include "OpenGLES2Interface.h"
	#include "OpenGLES2VertexArrayObject.h"
	#include "OpenGLES2Shader.h"

	using BackendGLInterface = OpenGLES2Interface;
	using BackendGLVAO = OpenGLES2VertexArrayObject;
	using BackendGLShader = OpenGLES2Shader;
#elif defined(MCENGINE_FEATURE_GLES32)
	#include "OpenGLES32Interface.h"
	#include "OpenGLES32VertexArrayObject.h"
	#include "OpenGLES32Shader.h"

	using BackendGLInterface = OpenGLES32Interface;
	using BackendGLVAO = OpenGLES32VertexArrayObject;
	using BackendGLShader = OpenGLES32Shader;
#elif defined(MCENGINE_FEATURE_GL3)
	#include "OpenGL3Interface.h"
	#include "OpenGL3VertexArrayObject.h"
	#include "OpenGLShader.h"

	using BackendGLInterface = OpenGL3Interface;
	using BackendGLVAO = OpenGL3VertexArrayObject;
	using BackendGLShader = OpenGLShader;
#endif

#endif
