//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		a collection of all necessary OpenGL header files
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
#include <gl/glew.h>

#define WGL_WGLEXT_PROTOTYPES // only used for wglDX*() interop stuff atm
#include <gl/wglew.h>

#include <gl/glu.h>
#include <gl/gl.h>

#include <gl/wglext.h>
#include <gl/glext.h>

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

#ifdef __SWITCH__

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#endif

#endif
