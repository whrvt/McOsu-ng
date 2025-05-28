//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		state class and listeners (FIFO)
//
// $NoKeywords: $key
//===============================================================================//

#include "Keyboard.h"

#include "Engine.h"

Keyboard::Keyboard() : InputDevice()
{
	m_bControlDown = false;
	m_bAltDown = false;
	m_bShiftDown = false;
	m_bSuperDown = false;
}

void Keyboard::addListener(KeyboardListener *keyboardListener, bool insertOnTop)
{
	if (keyboardListener == NULL)
	{
		engine->showMessageError("Keyboard Error", "addListener(NULL)!");
		return;
	}

	if (insertOnTop)
		m_listeners.insert(m_listeners.begin(), keyboardListener);
	else
		m_listeners.push_back(keyboardListener);
}

void Keyboard::removeListener(KeyboardListener *keyboardListener)
{
	for (size_t i=0; i<m_listeners.size(); i++)
	{
		if (m_listeners[i] == keyboardListener)
		{
			m_listeners.erase(m_listeners.begin() + i);
			i--;
		}
	}
}

void Keyboard::reset()
{
	m_bControlDown = false;
	m_bAltDown = false;
	m_bShiftDown = false;
	m_bSuperDown = false;
}

void Keyboard::onKeyDown(KEYCODE keyCode)
{
	switch (keyCode)
	{
	case KEY_LCONTROL:
	case KEY_RCONTROL:
		m_bControlDown = true;
		break;
	case KEY_LALT:
	case KEY_RALT:
		m_bAltDown = true;
		break;
	case KEY_LSHIFT:
	case KEY_RSHIFT:
		m_bShiftDown = true;
		break;
	case KEY_LSUPER:
	case KEY_RSUPER:
		m_bSuperDown = true;
		break;
	}

	KeyboardEvent e(keyCode);

	for (auto & m_listener : m_listeners)
	{
		m_listener->onKeyDown(e);
		if (e.isConsumed())
			break;
	}
}

void Keyboard::onKeyUp(KEYCODE keyCode)
{
	switch (keyCode)
	{
	case KEY_LCONTROL:
	case KEY_RCONTROL:
		m_bControlDown = false;
		break;
	case KEY_LALT:
	case KEY_RALT:
		m_bAltDown = false;
		break;
	case KEY_LSHIFT:
	case KEY_RSHIFT:
		m_bShiftDown = false;
		break;
	case KEY_LSUPER:
	case KEY_RSUPER:
		m_bSuperDown = false;
		break;
	}

	KeyboardEvent e(keyCode);

	for (auto & m_listener : m_listeners)
	{
		m_listener->onKeyUp(e);
		if (e.isConsumed())
			break;
	}
}

void Keyboard::onChar(KEYCODE charCode)
{
	KeyboardEvent e(charCode);

	for (auto & m_listener : m_listeners)
	{
		m_listener->onChar(e);
		if (e.isConsumed())
			break;
	}
}
