//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		base class for UI elements to inherit from, gui-window-wide
//
// $NoKeywords: $windowui
//===============================================================================//

#pragma once

#include "CBaseUIElement.h"
#include "CBaseUIWindow.h"

class WindowUIElement : public CBaseUIElement
{
public:
	using CBaseUIElement::CBaseUIElement;

	enum elemTypeWindow : TypeId
	{
		CONSOLEBOX = ENGINE_TYPES_START,
		VISUALPROFILER,
		VSCONTROLBAR,
		VSMUSICBROWSER,
		VSTITLEBAR,
		WINDOW_ELEMENT_END,
		VINYLSCRATCHERWINDOW
	};
};
