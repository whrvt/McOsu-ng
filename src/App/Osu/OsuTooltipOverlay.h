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
	~OsuTooltipOverlay() override;

	void draw() override;
	void update() override;

	void begin();
	void addLine(const UString& text);
	void end();

private:
	float m_fAnim;
	std::vector<UString> m_lines;

	bool m_bDelayFadeout;
};

#endif
