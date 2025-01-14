//================ Copyright (c) 2012, PG, All rights reserved. =================//
//
// Purpose:		windows context menu interface
//
// $NoKeywords: $wincmenu
//===============================================================================//

// TODO: DEPRECATED

#if defined(_WIN32) || defined(_WIN64) || defined(SDL_PLATFORM_WIN32) || defined(__CYGWIN__) || defined(__CYGWIN32__) || defined(__TOS_WIN__) || defined(SDL_PLATFORM_WINDOWS)

#ifndef WINCONTEXTMENU_H
#define WINCONTEXTMENU_H

#include "ContextMenu.h"
#include <Windows.h>

class WinContextMenu : public ContextMenu
{
public:
	WinContextMenu();
	virtual ~WinContextMenu();

	void begin();
	void addItem(UString text, int returnValue);
	void addSeparator();
	int end();

private:
	HMENU m_menu;
};

#endif

#endif
