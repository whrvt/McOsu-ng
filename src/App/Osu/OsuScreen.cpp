//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		baseclass for any drawable screen state object of the game
//
// $NoKeywords: $
//===============================================================================//

#include "OsuScreen.h"

#include "Engine.h"
#include "Keyboard.h"

#include "Osu.h"
#include "OsuOptionsMenu.h"

OsuScreen::OsuScreen()
{
	m_bVisible = false;
}

void OsuScreen::onKeyDown(KeyboardEvent &e)
{
	if (!m_bVisible) return;

	// global hotkey
	if (e == KEY_O && keyboard->isControlDown())
	{
		osu->toggleOptionsMenu();
		e.consume();
	}
}
