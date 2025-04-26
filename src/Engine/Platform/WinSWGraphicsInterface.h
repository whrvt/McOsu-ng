//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		windows software rasterizer graphics interface
//
// $NoKeywords: $winswi
//===============================================================================//

#pragma once
#ifndef WINSWGRAPHICSINTERFACE_H
#define WINSWGRAPHICSINTERFACE_H

#include "SWGraphicsInterface.h"
#ifdef MCENGINE_FEATURE_SOFTRENDERER
#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__CYGWIN__) || defined(__CYGWIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__)

#include <windows.h>

class WinSWGraphicsInterface : public SWGraphicsInterface
{
public:
	WinSWGraphicsInterface(HWND hwnd);
	virtual ~WinSWGraphicsInterface();

	// scene
	void endScene();

	// device settings
	void setVSync(bool vsync);

private:
	// device context
	HWND m_hwnd;
	HDC m_hdc;
};

#endif
#else
class WinSWGraphicsInterface : public SWGraphicsInterface{};
#endif
#endif
