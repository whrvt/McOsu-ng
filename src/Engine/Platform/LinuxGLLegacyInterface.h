//================ Copyright (c) 2014, PG, All rights reserved. =================//
//
// Purpose:		linux opengl interface
//
// $NoKeywords: $linuxgli
//===============================================================================//

#pragma once
#ifndef LINUXGLINTERFACE_H
#define LINUXGLINTERFACE_H

typedef unsigned char BYTE;

#include "OpenGLLegacyInterface.h"

#ifdef __linux__

#if defined(MCENGINE_FEATURE_OPENGL) && !defined(MCENGINE_FEATURE_SDL)

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "OpenGLHeaders.h"

XVisualInfo *getVisualInfo(Display *display);

class LinuxGLLegacyInterface : public OpenGLLegacyInterface
{
public:
	LinuxGLLegacyInterface(Display *display, Window window);
	virtual ~LinuxGLLegacyInterface();

	// scene
	void endScene();

	// device settings
	void setVSync(bool vsync);

	// ILLEGAL:
	inline GLXContext getGLXContext() const {return m_glc;}

private:
	Display *m_display;
	Window m_window;
	GLXContext m_glc;
};

#endif

#else
class LinuxGLLegacyInterface : public OpenGLLegacyInterface{};
#endif

#endif
