//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		a wrapper for consumable keyboard events
//
// $NoKeywords: $key
//===============================================================================//

#pragma once
#ifndef KEYBOARDEVENT_H
#define KEYBOARDEVENT_H

#include <cstdint>
using KEYCODE = uint_fast16_t;

class KeyboardEvent
{
public:
	KeyboardEvent(KEYCODE keyCode);

	void consume();

	[[nodiscard]] inline bool isConsumed() const {return m_bConsumed;}
	[[nodiscard]] inline KEYCODE getKeyCode() const {return m_keyCode;}
	[[nodiscard]] inline KEYCODE getCharCode() const {return m_keyCode;}

	inline bool operator == (const KEYCODE &rhs) const {return m_keyCode == rhs;}
	inline bool operator != (const KEYCODE &rhs) const {return m_keyCode != rhs;}

	explicit operator KEYCODE() const {return m_keyCode;}

private:
	KEYCODE m_keyCode;
	bool m_bConsumed;
};

#endif
