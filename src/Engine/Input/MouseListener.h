//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		simple listener class for mouse events
//
// $NoKeywords: $mouse
//===============================================================================//

#pragma once
#ifndef MOUSELISTENER_H
#define MOUSELISTENER_H

namespace MouseButton
{
	enum Index : unsigned char
	{
		BUTTON_NONE = 0,
		BUTTON_LEFT = 1,
		BUTTON_MIDDLE = 2,
		BUTTON_RIGHT = 3,
		BUTTON_X1 = 4,
		BUTTON_X2 = 5,
		BUTTON_COUNT = 6
	};
};

using enum MouseButton::Index;

class MouseListener
{
public:
	virtual ~MouseListener() {;}

	virtual void onButtonChange(MouseButton::Index button, bool down) {;}

	virtual void onWheelVertical(int delta) {;}
	virtual void onWheelHorizontal(int delta) {;}
};

#endif
