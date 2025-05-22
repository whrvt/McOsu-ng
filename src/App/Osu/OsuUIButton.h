//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		generic button (context menu items, mod selection screen, etc.)
//
// $NoKeywords: $osubt
//===============================================================================//

#pragma once
#ifndef OSUBUTTON_H
#define OSUBUTTON_H

#include "OsuUIElement.h"

class Osu;

class OsuUIButton : public CBaseUIButton
{
public:
	OsuUIButton(float xPos, float yPos, float xSize, float ySize, UString name, UString text);

	void draw(Graphics *g) override;
	void update() override;

	void setColor(Color color) {m_color = color; m_backupColor = color;}
	void setUseDefaultSkin() {m_bDefaultSkin = true;}
	void setAlphaAddOnHover(float alphaAddOnHover) {m_fAlphaAddOnHover = alphaAddOnHover;}

	void setTooltipText(UString text);

	void onMouseInside() override;
	void onMouseOutside() override;

	void animateClickColor();

	// inspection
	CBASE_UI_TYPE(OsuUIButton, OsuUIElement::OSUUIBUTTON, CBaseUIButton)
private:
	void onClicked() override;
	void onFocusStolen() override;

	bool m_bDefaultSkin;
	Color m_color;
	Color m_backupColor;
	float m_fBrightness;
	float m_fAnim;
	float m_fAlphaAddOnHover;

	std::vector<UString> m_tooltipTextLines;
	bool m_bFocusStolenDelay;
};

#endif
