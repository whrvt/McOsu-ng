//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		simple listener class for mouse events
//
// $NoKeywords: $mouse
//===============================================================================//

#pragma once
#ifndef MOUSELISTENER_H
#define MOUSELISTENER_H

class MouseListener
{
public:
	virtual ~MouseListener() {;}

	virtual void onButtonChange(int button, bool down) {;}

	virtual void onLeftChange(bool down) {;}
	virtual void onMiddleChange(bool down) {;}
	virtual void onRightChange(bool down) {;}
	virtual void onButton4Change(bool down) {;}
	virtual void onButton5Change(bool down) {;}

	virtual void onWheelVertical(int delta) {;}
	virtual void onWheelHorizontal(int delta) {;}
};

#endif
