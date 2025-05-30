//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		tooltips
//
// $NoKeywords: $osutt
//===============================================================================//

#pragma once
#ifndef OSUTOOLTIPOVERLAY_H
#define OSUTOOLTIPOVERLAY_H

#include "OsuScreen.h"

class Osu;

class OsuTooltipOverlay : public OsuScreen
{
public:
	OsuTooltipOverlay();
	virtual ~OsuTooltipOverlay();

	virtual void draw();
	virtual void update();

	void begin();
	void addLine(UString text);
	void end();

private:
	float m_fAnim;
	std::vector<UString> m_lines;

	bool m_bDelayFadeout;
};

#endif
