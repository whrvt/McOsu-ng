//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		big ass back button (options, songbrowser)
//
// $NoKeywords: $osubbt
//===============================================================================//

#pragma once
#ifndef OSUUIBACKBUTTON_H
#define OSUUIBACKBUTTON_H

#include "CBaseUIButton.h"

class Osu;

class OsuUIBackButton : public CBaseUIButton
{
public:
	OsuUIBackButton(float xPos, float yPos, float xSize, float ySize, UString name);

	virtual void draw(Graphics *g);
	virtual void update();

	virtual void onMouseInside();
	virtual void onMouseOutside();

	virtual void updateLayout();

	void resetAnimation();

private:
	float m_fAnimation;
	float m_fImageScale;
};
#endif
