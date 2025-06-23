//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		generic checkbox
//
// $NoKeywords: $osucb
//===============================================================================//

#pragma once
#ifndef OSUUICHECKBOX_H
#define OSUUICHECKBOX_H

#include "CBaseUICheckbox.h"

class Osu;

class OsuUICheckbox : public CBaseUICheckbox
{
public:
	OsuUICheckbox(float xPos, float yPos, float xSize, float ySize, UString name, UString text);

	void update() override;

	void setTooltipText(UString text);

private:
	void onFocusStolen() override;

	std::vector<UString> m_tooltipTextLines;

	bool m_bFocusStolenDelay;
};

#endif
