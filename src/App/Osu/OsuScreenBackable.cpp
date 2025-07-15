//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		screen + back button
//
// $NoKeywords: $
//===============================================================================//

#include "OsuScreenBackable.h"

#include "Keyboard.h"
#include "ResourceManager.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuKeyBindings.h"

#include "OsuUIBackButton.h"

OsuScreenBackable::OsuScreenBackable() : OsuScreen()
{
	m_backButton = new OsuUIBackButton(-1, 0, 0, 0, "");
	m_backButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuScreenBackable::onBack) );

	updateLayout();
}

OsuScreenBackable::~OsuScreenBackable()
{
	SAFE_DELETE(m_backButton);
}

void OsuScreenBackable::draw()
{
	if (!m_bVisible) return;

	m_backButton->draw();
}

void OsuScreenBackable::update()
{
	if (!m_bVisible) return;

	m_backButton->update();
}

void OsuScreenBackable::onKeyDown(KeyboardEvent &e)
{
	OsuScreen::onKeyDown(e);
	if (!m_bVisible || e.isConsumed()) return;

	if (e == KEY_ESCAPE || e == cv::osu::keybinds::GAME_PAUSE.getVal<KEYCODE>())
		onBack();

	e.consume();
}

void OsuScreenBackable::onKeyUp(KeyboardEvent &e)
{
	if (!m_bVisible) return;

	e.consume();
}

void OsuScreenBackable::onChar(KeyboardEvent &e)
{
	if (!m_bVisible) return;

	e.consume();
}

void OsuScreenBackable::stealFocus()
{
	m_backButton->stealFocus();
}

void OsuScreenBackable::updateLayout()
{
	m_backButton->updateLayout();
	m_backButton->setPosY(osu->getVirtScreenHeight() - m_backButton->getSize().y);
}

void OsuScreenBackable::onResolutionChange(Vector2  /*newResolution*/)
{
	updateLayout();
}
