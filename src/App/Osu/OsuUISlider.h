//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		generic slider (mod overrides, options, etc.)
//
// $NoKeywords: $osusl
//===============================================================================//

#pragma once
#ifndef OSUUISLIDER_H
#define OSUUISLIDER_H

#include "OsuUIElement.h"

#include "CBaseUISlider.h"

class Osu;

class OsuUISlider : public CBaseUISlider
{
public:
	OsuUISlider(float xPos, float yPos, float xSize, float ySize, UString name);

	void draw() override;

	// inspection
	CBASE_UI_TYPE(OsuUISlider, OsuUIElement::OSUUISLIDER, CBaseUISlider)
};

#endif
