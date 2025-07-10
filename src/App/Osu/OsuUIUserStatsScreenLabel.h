//================ Copyright (c) 2022, PG, All rights reserved. =================//
//
// Purpose:		currently only used for showing the pp/star algorithm versions
//
// $NoKeywords: $
//===============================================================================//

#pragma once
#ifndef OSUUIUSERSTATSSCREENLABEL_H
#define OSUUIUSERSTATSSCREENLABEL_H

#include "CBaseUILabel.h"

class Osu;

class OsuUIUserStatsScreenLabel : public CBaseUILabel
{
public:
	OsuUIUserStatsScreenLabel(float xPos=0, float yPos=0, float xSize=0, float ySize=0, const UString& name="", const UString& text="");

	void update() override;

	void setTooltipText(const UString& text) {m_tooltipTextLines = text.split("\n");}

private:
	std::vector<UString> m_tooltipTextLines;
};

#endif
