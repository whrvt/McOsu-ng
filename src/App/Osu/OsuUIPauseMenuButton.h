//================ Copyright (c) 2018, PG, All rights reserved. =================//
//
// Purpose:		pause menu button
//
// $NoKeywords: $
//===============================================================================//

#pragma once
#ifndef OSUUIPAUSEMENUBUTTON_H
#define OSUUIPAUSEMENUBUTTON_H

#include "CBaseUIButton.h"

class Osu;

class Image;

class OsuUIPauseMenuButton : public CBaseUIButton
{
public:
	OsuUIPauseMenuButton(std::function<Image*()> getImageFunc, float xPos, float yPos, float xSize, float ySize, UString name);

	void draw() override;

	void onMouseInside() override;
	void onMouseOutside() override;

	void setBaseScale(float xScale, float yScale);
	void setAlpha(float alpha) {m_fAlpha = alpha;}

	Image *getImage() {return getImageFunc != NULL ? getImageFunc() : NULL;}

private:
	Vector2 m_vScale;
	Vector2 m_vBaseScale;
	float m_fScaleMultiplier;

	float m_fAlpha;

	std::function<Image*()> getImageFunc;
};

#endif
