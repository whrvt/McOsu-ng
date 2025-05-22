//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		state class and listeners (FIFO)
//
// $NoKeywords: $key
//===============================================================================//

#pragma once
#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "InputDevice.h"
#include "KeyboardKeys.h"
#include "KeyboardEvent.h"
#include "KeyboardListener.h"

class Keyboard final : public InputDevice
{
public:
	Keyboard();
	~Keyboard() override {;}

	void addListener(KeyboardListener *keyboardListener, bool insertOnTop = false);
	void removeListener(KeyboardListener *keyboardListener);
	void reset();

	void onKeyDown(KEYCODE keyCode);
	void onKeyUp(KEYCODE keyCode);
	void onChar(KEYCODE charCode);

	[[nodiscard]] inline bool isControlDown() const {return m_bControlDown;}
	[[nodiscard]] inline bool isAltDown() const {return m_bAltDown;}
	[[nodiscard]] inline bool isShiftDown() const {return m_bShiftDown;}
	[[nodiscard]] inline bool isSuperDown() const {return m_bSuperDown;}

private:
	bool m_bControlDown;
	bool m_bAltDown;
	bool m_bShiftDown;
	bool m_bSuperDown;

	std::vector<KeyboardListener*> m_listeners;
};

#endif
